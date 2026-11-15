package org.rob_smallshire.oaknut.adfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.BufferStorage;
import org.rob_smallshire.oaknut.discimage.DiscImage;
import org.rob_smallshire.oaknut.discimage.SectorsView;
import org.rob_smallshire.oaknut.discimage.Storage;
import org.rob_smallshire.oaknut.discimage.UnifiedDisc;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornEncoding;
import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.filesystem.ValidationFinding;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Standalone floppy ADFS handle. Paths resolve against live directory data on every operation. */
public final class Adfs implements AutoCloseable
{
    private final Storage storage;
    private final boolean rollbackEnabled;
    private final AdfsFormat format;
    private final UnifiedDisc disc;
    private final FileChannel channel;
    private final MappedByteBuffer mapping;
    private OldFreeSpaceMap oldMap;
    private NewMap newMap;
    private int rootAddress;
    private boolean closed;
    private boolean inTransaction;

    private Adfs(
            ByteBuffer buffer,
            AdfsFormat format,
            FileChannel channel,
            MappedByteBuffer mapping) throws AdfsException
    {
        this(new BufferStorage(buffer), format, channel, mapping, true);
    }

    private Adfs(
            Storage storage,
            AdfsFormat format,
            FileChannel channel,
            MappedByteBuffer mapping,
            boolean rollbackEnabled) throws AdfsException
    {
        this.storage = storage;
        this.rollbackEnabled = rollbackEnabled;
        this.format = format;
        this.channel = channel;
        this.mapping = mapping;
        disc = new UnifiedDisc(new DiscImage(storage, format.surfaces()));
        loadMaps();
    }

    public static Adfs create(AdfsFormat format) throws AdfsException
    {
        return create(format, "", 0, 0);
    }

    public static Adfs create(AdfsFormat format, String title) throws AdfsException
    {
        return create(format, title, 0, 0);
    }

    public static Adfs create(AdfsFormat format, String title, int boot, int id)
            throws AdfsException
    {
        if (boot < 0 || boot > 3)
            throw new IllegalArgumentException("Invalid boot option");
        if (format.big())
            AcornEncoding.checkTitle(title, 10, false);
        else
            format.directoryFormat.validateTitle(title);
        ByteBuffer buffer = ByteBuffer.allocate(format.size());
        UnifiedDisc disc = new UnifiedDisc(new DiscImage(buffer, format.surfaces()));
        int root;
        if (format.newMap())
        {
            DiscRecord record = DiscRecord.forFormat(
                    format,
                    title.length() > 10 ? title.substring(0, 10) : title,
                    id,
                    boot);
            root = NewMap.format(disc.bytes(0, format.size()), record);
            Directory dir = new Directory(
                    "$",
                    format.big() ? "" : title,
                    record.root(),
                    record.root(),
                    List.of(),
                    0,
                    format.big() ? "SBPr" : "Nick",
                    format.big() ? 2048 : 0);
            disc
                    .bytes(root, format.directoryFormat.size())
                    .write(0, format.directoryFormat.serialize(dir));
        } else
        {
            SectorsView map = disc.bytes(0, 512);
            root = format == AdfsFormat.D ? 4 : 2;
            int used = root + format.directoryFormat.size() / 256;
            map.writeLe(0, used, 3);
            map.writeLe(256, format.size() / 256 - used, 3);
            map.writeLe(252, format.size() / 256, 3);
            map.writeLe(507, id, 2);
            map.put(509, boot);
            map.put(510, 3);
            map.put(255, OldFreeSpaceMap.checksum(map, 0));
            map.put(511, OldFreeSpaceMap.checksum(map, 256));
            Directory dir = new Directory(
                    "$",
                    title,
                    root,
                    root,
                    List.of(),
                    0,
                    format == AdfsFormat.D ? "Nick" : "Hugo",
                    0);
            byte[] rootBytes = format.directoryFormat.serialize(dir);
            if (format.directoryFormat == DirectoryFormat.OLD)
            {
                java.util.Arrays.fill(rootBytes, 0x4cc, 0x4d6, (byte) 0);
                rootBytes[0x4cc] = '$';
                java.util.Arrays.fill(rootBytes, 0x4d9, 0x4ec, (byte) 0);
                byte[] titleBytes = AcornEncoding.encode(title, true);
                System.arraycopy(titleBytes, 0, rootBytes, 0x4d9, titleBytes.length);
            }
            disc.bytes(root * 256, format.directoryFormat.size()).write(0, rootBytes);
        }
        return new Adfs(buffer, format, null, null);
    }

    public static Adfs fromBuffer(byte[] buffer) throws AdfsException
    {
        return fromBuffer(ByteBuffer.wrap(buffer));
    }

    public static Adfs fromBuffer(ByteBuffer buffer) throws AdfsException
    {
        return detect(buffer, false, null, null);
    }

    public static Adfs fromBuffer(ByteBuffer buffer, boolean preferSequential) throws AdfsException
    {
        return detect(buffer, preferSequential, null, null);
    }

    public static Adfs fromBuffer(ByteBuffer buffer, AdfsFormat format) throws AdfsException
    {
        Adfs result = new Adfs(buffer, format, null, null);
        result.readDirectory(result.rootAddress);
        return result;
    }

    /* External storage is borrowed. Disable rollback when its owner supplies transactions.
     * Views report subsequent storage failures as UncheckedIOException; opening unwraps them. */
    public static Adfs fromStorage(Storage storage, boolean rollbackEnabled) throws IOException
    {
        try
        {
            return detect(storage, false, null, null, rollbackEnabled);
        } catch (UncheckedIOException e)
        {
            throw e.getCause();
        }
    }

    public static Adfs fromStorage(Storage storage, AdfsFormat format, boolean rollbackEnabled)
            throws IOException
    {
        try
        {
            Adfs result = new Adfs(storage, format, null, null, rollbackEnabled);
            result.readDirectory(result.rootAddress);
            return result;
        } catch (UncheckedIOException e)
        {
            throw e.getCause();
        }
    }

    private static Adfs detect(
            ByteBuffer buffer,
            boolean sequential,
            FileChannel channel,
            MappedByteBuffer mapping) throws AdfsException
    {
        return detect(new BufferStorage(buffer), sequential, channel, mapping, true);
    }

    private static Adfs detect(
            Storage storage,
            boolean sequential,
            FileChannel channel,
            MappedByteBuffer mapping,
            boolean rollbackEnabled) throws AdfsException
    {
        int size = storage.size();
        AdfsException last = null;
        List<AdfsFormat> candidates = new ArrayList<>();
        for (AdfsFormat f : AdfsFormat.values())
            if (f.size() == size)
                candidates.add(f);
        /* A truncated old-map image is described by its map, not the file length. */
        if (size >= 512)
        {
            int declared = (int) new SectorsView(storage, 0, size).readLe(252, 3) * 256;
            if (declared > size)
            {
                candidates.clear();
                for (AdfsFormat f : AdfsFormat.values())
                    if (!f.newMap() && f.size() == declared && !candidates.contains(f))
                        candidates.add(f);
            }
        }
        if (sequential && candidates.contains(AdfsFormat.L_SEQUENTIAL))
        {
            candidates.remove(AdfsFormat.L_SEQUENTIAL);
            candidates.addFirst(AdfsFormat.L_SEQUENTIAL);
        }
        Adfs best = null;
        int bestCount = -1;
        for (AdfsFormat f : candidates)
        {
            try
            {
                Adfs adfs = new Adfs(storage, f, channel, mapping, rollbackEnabled);
                adfs.readDirectory(adfs.rootAddress);
                int count = adfs.directoryScore();
                if (count >= 1000000)
                    return adfs;
                if (count > bestCount)
                {
                    best = adfs;
                    bestCount = count;
                }
            } catch (AdfsException | IllegalArgumentException e)
            {
                last = e instanceof AdfsException a ? a : new AdfsFormatException(e.getMessage());
            }
        }
        if (best != null)
            return best;
        throw new AdfsFormatException(
                "Unrecognised ADFS floppy image" + (last == null ? "" : ": " + last.getMessage()));
    }

    public static Adfs fromFile(Path path) throws IOException
    {
        return fromFile(path, false);
    }

    public static Adfs fromFile(Path path, boolean readOnly) throws IOException
    {
        return fromFile(path, readOnly, null);
    }

    public static Adfs fromFile(Path path, boolean readOnly, AdfsFormat explicitFormat)
            throws IOException
    {
        FileChannel channel = readOnly ?
                FileChannel.open(path, StandardOpenOption.READ) :
                FileChannel.open(path, StandardOpenOption.READ, StandardOpenOption.WRITE);
        try
        {
            MappedByteBuffer mapping = channel.map(
                    readOnly ? FileChannel.MapMode.READ_ONLY : FileChannel.MapMode.READ_WRITE,
                    0,
                    channel.size());
            if (explicitFormat != null)
            {
                Adfs result = new Adfs(mapping, explicitFormat, channel, mapping);
                result.readDirectory(result.rootAddress);
                return result;
            }
            return detect(
                    mapping,
                    path.toString().toLowerCase(java.util.Locale.ROOT).endsWith(".adf"),
                    channel,
                    mapping);
        } catch (IOException | RuntimeException e)
        {
            channel.close();
            throw e;
        }
    }

    public static Adfs createFile(Path path, AdfsFormat format, String title) throws IOException
    {
        return createFile(path, format, title, 0);
    }

    public static Adfs createFile(Path path, AdfsFormat format, String title, int bootOption)
            throws IOException
    {
        try (Adfs blank = create(format, title, bootOption, 0))
        {
            Files.write(path, blank.toByteArray(), StandardOpenOption.CREATE_NEW);
        }
        return fromFile(path, false, format);
    }

    public static Adfs createFile(Path path) throws IOException
    {
        String name = path.getFileName().toString();
        int dot = name.lastIndexOf('.');
        return createFile(path, AdfsFormat.forExtension(dot < 0 ? "" : name.substring(dot)), "");
    }

    private static List<DirectoryEntry> sorted(List<DirectoryEntry> entries)
    {
        return entries
                .stream()
                .sorted(Comparator.comparing(e -> DirectoryFormat.nameKey(e.name())))
                .toList();
    }

    private static List<String> keys(List<String> parts)
    {
        return parts.stream().map(DirectoryFormat::nameKey).toList();
    }

    private static int copyTree(AdfsPath source, AdfsPath target) throws AdfsException
    {
        int count = 0;
        for (AdfsPath path : source.list())
        {
            AdfsPath dest = target.resolve(List.of(path.name()));
            if (path.isDirectory())
            {
                dest.mkdir();
                dest.setTitle(path.title());
                count += copyTree(path, dest);
            } else
                dest.writeBytes(path.readBytes(), path.stat().metadata());
            dest.setMetadata(path.stat().metadata());
            count++;
        }
        return count;
    }

    private void loadMaps() throws AdfsException
    {
        try
        {
            if (format.newMap())
            {
                DiscRecord dr = DiscRecord.parse(disc.bytes(format.zones == 1 ? 4 : 0xdc0, 60), 0);
                if (!dr.looksValid() || dr.density() == 0 || dr.discSize() != format.size())
                    throw new AdfsFormatException("Unsupported floppy disc record");
                int boot = dr.bootmap();
                dr = DiscRecord.parse(disc.bytes(boot + 4, 60), 0);
                newMap = new NewMap(disc.bytes(boot, dr.zones() * dr.sectorSize() * 2), dr, disc);
                if (!newMap.validate().isEmpty())
                    throw new AdfsMapException("Invalid zone checksum");
                rootAddress = dr.root();
            } else
            {
                oldMap = new OldFreeSpaceMap(disc.bytes(0, 512));
                rootAddress = format == AdfsFormat.D ? 4 : 2;
            }
        } catch (IndexOutOfBoundsException | ArithmeticException e)
        {
            throw new AdfsFormatException("Truncated or malformed floppy image");
        }
    }

    public AdfsFormat format()
    {
        return format;
    }

    public AdfsGeometry geometry()
    {
        return AdfsGeometry.of(format);
    }

    public AdfsPath root()
    {
        return new AdfsPath(this, List.of());
    }

    public AdfsPath path(String path)
    {
        return root().resolve(path);
    }

    /* Components are literal names relative to the root; no Acorn separators or special
     * directory tokens are interpreted. This supports callers with their own path syntax. */
    public AdfsPath path(List<String> components)
    {
        return new AdfsPath(this, components);
    }

    public boolean closed()
    {
        return closed;
    }

    public boolean readOnly()
    {
        return storage.isReadOnly();
    }

    public boolean usesTypedMetadata()
    {
        return format.directoryFormat != DirectoryFormat.OLD;
    }

    public byte[] toByteArray() throws AdfsException
    {
        requireOpen();
        return new SectorsView(storage, 0, storage.size()).toByteArray();
    }

    public int totalSize() throws AdfsException
    {
        requireOpen();
        return newMap == null ? oldMap.totalSize() : newMap.discRecord().discSize();
    }

    public int freeSpace() throws AdfsException
    {
        requireOpen();
        return newMap == null ? oldMap.freeSpace() : newMap.freeSpace();
    }

    public String discName() throws AdfsException
    {
        requireOpen();
        return newMap == null ? oldMap.discName() : newMap.discRecord().discName();
    }

    public int bootOption() throws AdfsException
    {
        requireOpen();
        return newMap == null ? oldMap.bootOption() : newMap.discRecord().bootOption();
    }

    public String title() throws AdfsException
    {
        return format.big() ? discName() : root().title();
    }

    public void setTitle(String title) throws AdfsException
    {
        if (!format.big())
        {
            root().setTitle(title);
            return;
        }
        AcornEncoding.checkTitle(title, 10, false);
        transaction(() -> newMap.setDiscName(title));
    }

    public void setBootOption(int option) throws AdfsException
    {
        if (option < 0 || option > 3)
            throw new IllegalArgumentException("Invalid boot option");
        requireOpen();
        if (newMap != null)
            throw new AdfsException("Setting boot option on new maps is unsupported upstream");
        transaction(() -> oldMap.setBootOption(option));
    }

    void requireOpen() throws AdfsClosedException
    {
        if (closed)
            throw new AdfsClosedException("Filesystem is closed");
    }

    private void requireWritable() throws AdfsException
    {
        requireOpen();
        if (readOnly())
            throw new AdfsReadOnlyException("Filesystem is read-only");
    }

    void transaction(Mutation mutation) throws AdfsException
    {
        requireWritable();
        if (inTransaction)
        {
            mutation.run();
            return;
        }
        byte[] snapshot = rollbackEnabled ? toByteArray() : null;
        int previousRoot = rootAddress;
        inTransaction = true;
        try
        {
            mutation.run();
        } catch (AdfsException | RuntimeException e)
        {
            if (rollbackEnabled)
            {
                new SectorsView(storage, 0, storage.size()).write(0, snapshot);
                rootAddress = previousRoot;
                loadMaps();
            }
            throw e;
        } finally
        {
            inTransaction = false;
        }
    }

    public void flush() throws IOException
    {
        requireOpen();
        storage.flush();
        if (mapping != null && !readOnly())
            mapping.force();
    }

    @Override
    public void close() throws IOException
    {
        if (closed)
            return;
        try
        {
            flush();
        } finally
        {
            closed = true;
            if (channel != null)
                channel.close();
        }
    }

    /* All directory and data access shares the same old/new-map address translation. */
    byte[] readObject(int address, int size) throws AdfsException
    {
        requireOpen();
        try
        {
            return newMap == null ?
                    disc.bytes(address * 256, size).toByteArray() :
                    newMap.readObject(address, size);
        } catch (IndexOutOfBoundsException e)
        {
            throw new AdfsBeyondImageException("Object lies beyond image: " + address);
        }
    }

    void writeObject(int address, byte[] bytes) throws AdfsException
    {
        try
        {
            if (newMap == null)
            {
                int paddedSize = (bytes.length + 255) / 256 * 256;
                disc
                        .bytes(address * 256, paddedSize)
                        .write(0, java.util.Arrays.copyOf(bytes, paddedSize));
            } else
                newMap.writeObjectData(address, bytes);
        } catch (IndexOutOfBoundsException e)
        {
            throw new AdfsBeyondImageException("Object lies beyond image: " + address);
        }
    }

    Directory readDirectory(int address) throws AdfsException
    {
        int size = format.directoryFormat.size();
        if (format.big())
        {
            long declared = SectorsView.wrap(readObject(address, 28)).readLe(12, 4);
            if (declared > format.size())
                throw new AdfsDirectoryException("Invalid directory size " + declared);
            size = (int) declared;
        }
        if (size < format.directoryFormat.size() || size > format.size())
            throw new AdfsDirectoryException("Invalid directory size " + size);
        Directory result =
                format.directoryFormat.parse(SectorsView.wrap(readObject(address, size)), address);
        String previous = null;
        for (DirectoryEntry e : result.entries())
        {
            String key = DirectoryFormat.nameKey(e.name());
            if (previous != null && previous.compareTo(key) >= 0)
                throw new AdfsDirectoryException("Directory entries are unsorted or duplicated");
            previous = key;
        }
        return result;
    }

    Directory directory(List<String> parts) throws AdfsException
    {
        Directory dir = readDirectory(rootAddress);
        for (String part : parts)
        {
            DirectoryEntry e = dir.find(part);
            if (e == null || !e.directory())
                throw new AdfsPathException("Not a directory: " + part);
            dir = readDirectory(e.discAddress());
        }
        return dir;
    }

    Directory parent(List<String> parts) throws AdfsException
    {
        if (parts.isEmpty())
            throw new AdfsPathException("Operation cannot replace root");
        return directory(parts.subList(0, parts.size() - 1));
    }

    DirectoryEntry entry(List<String> parts) throws AdfsException
    {
        if (parts.isEmpty())
        {
            Directory root = directory(parts);
            return new DirectoryEntry(
                    "$",
                    0,
                    0,
                    format.directoryFormat.size(),
                    rootAddress,
                    root.sequence(),
                    Access.WR | Access.PR,
                    true);
        }
        DirectoryEntry e = parent(parts).find(parts.getLast());
        if (e == null)
            throw new AdfsPathException("No such entry: " + parts.getLast());
        return e;
    }

    private int allocate(int size, boolean directory) throws AdfsException
    {
        if (newMap != null)
        {
            int address = newMap.allocateObject(size);
            return address;
        }
        if (size == 0)
            return 0;
        return oldMap.allocate(
                (size + 255) / 256,
                directory && format == AdfsFormat.D ? 4 : 1,
                storage.size() / 256);
    }

    private void release(Directory parent, DirectoryEntry entry) throws AdfsException
    {
        if (newMap == null)
        {
            if (entry.length() > 0)
                oldMap.free(entry.discAddress(), (entry.length() + 255) / 256);
        } else
        {
            int id = entry.discAddress() >>> 8;
            boolean shared = parent.discAddress() >>> 8 == id || parent
                    .entries()
                    .stream()
                    .anyMatch(e -> !e.name().equals(entry.name()) && e.discAddress() >>> 8 == id);
            if (!shared)
                newMap.freeObject(entry.discAddress());
        }
    }

    private int allocateFile(Directory parent, byte[] bytes) throws AdfsException
    {
        if (newMap != null && bytes.length > 0 &&
                bytes.length < newMap.minFragmentBytes() - format.sectorSize)
        {
            ArrayList<Integer> ids = new ArrayList<>();
            if ((parent.discAddress() & 255) == 1 && parent.discAddress() != rootAddress)
                ids.add(parent.discAddress() >>> 8);
            for (DirectoryEntry e : parent.entries())
                if (!e.directory() && (e.discAddress() & 255) == 1)
                    ids.add(e.discAddress() >>> 8);
            for (int id : ids)
            {
                if (newMap.fragmentCount(id) != 1)
                    continue;
                int usedEnd = parent.discAddress() >>> 8 == id ?
                        format.directoryFormat.size() / format.sectorSize :
                        0;
                for (DirectoryEntry e : parent.entries())
                    if (e.discAddress() >>> 8 == id)
                        usedEnd = Math.max(
                                usedEnd, Math.max(0, (e.discAddress() & 255) - 1) + Math.max(
                                        1,
                                        (e.length() + format.sectorSize - 1) / format.sectorSize));
                if (usedEnd + (bytes.length + format.sectorSize - 1) / format.sectorSize <=
                        newMap.fragmentCapacity(id) / format.sectorSize)
                {
                    int address = (id << 8) | (usedEnd + 1);
                    writeObject(address, bytes);
                    return address;
                }
            }
        }
        int address = allocate(bytes.length, false);
        if (newMap != null &&
                newMap.fragmentCapacity(address >>> 8) - bytes.length > format.sectorSize &&
                bytes.length < newMap.minFragmentBytes())
            address |= 1;
        writeObject(address, bytes);
        return address;
    }

    private int persist(Directory dir, List<String> parts) throws AdfsException
    {
        int address = dir.discAddress();
        if (format.big())
        {
            int needed = format.directoryFormat.requiredSize(dir, format.sectorSize);
            if (needed > dir.bigSize())
            {
                try
                {
                    newMap.growFragment(address, needed);
                } catch (AdfsDiscFullException e)
                {
                    int previous = address;
                    address = allocate(needed, true);
                    newMap.freeObject(previous);
                    if (parts.isEmpty())
                    {
                        rootAddress = address;
                        newMap.setRoot(address, needed);
                    } else
                    {
                        Directory parent = parent(parts);
                        int nextAddress = address;
                        List<DirectoryEntry> entries = parent
                                .entries()
                                .stream()
                                .map(entry -> DirectoryFormat
                                        .nameKey(entry.name())
                                        .equals(DirectoryFormat.nameKey(parts.getLast())) ?
                                        entry.withAddress(nextAddress, needed) :
                                        entry)
                                .toList();
                        persist(parent.withEntries(entries), parts.subList(0, parts.size() - 1));
                    }
                }
                dir = new Directory(
                        dir.name(),
                        dir.title(),
                        parts.isEmpty() ? address : dir.parentAddress(),
                        address,
                        dir.entries(),
                        dir.sequence(),
                        dir.signature(),
                        needed);
            }
        }
        writeObject(address, format.directoryFormat.serialize(dir));
        return address;
    }

    private void checkName(String name) throws AdfsPathException
    {
        try
        {
            format.directoryFormat.validateName(name);
        } catch (IllegalArgumentException e)
        {
            throw new AdfsPathException(e.getMessage());
        }
    }

    void writeFile(List<String> parts, byte[] bytes, AcornMeta meta) throws AdfsException
    {
        if (parts.isEmpty())
            throw new AdfsPathException("Cannot write to root directory");
        transaction(() -> {
            checkName(parts.getLast());
            Directory parent = parent(parts);
            DirectoryEntry existing = parent.find(parts.getLast());
            if (existing != null && existing.directory())
                throw new AdfsPathException("Cannot overwrite a directory");
            if (existing != null && (existing.access() & Access.L) != 0)
                throw new AdfsFileLockedException("File is locked");
            if (existing == null && parent.entries().size() >= format.directoryFormat.maxEntries())
                throw new AdfsDirectoryFullException("Directory full");
            if (existing != null)
                release(parent, existing);
            ArrayList<DirectoryEntry> entries = new ArrayList<>(parent.entries());
            entries.remove(existing);
            Directory allocationParent = parent.withEntries(entries);
            int address = allocateFile(allocationParent, bytes);
            int access = meta.access() != null ?
                    meta.access() :
                    existing == null ? Access.WR | Access.PR : existing.access();
            entries.add(new DirectoryEntry(
                    parts.getLast(),
                    meta.loadAddress(),
                    meta.executionAddress(),
                    bytes.length,
                    address,
                    0,
                    access,
                    false));
            persist(parent.withEntries(sorted(entries)), parts.subList(0, parts.size() - 1));
        });
    }

    void mkdir(List<String> parts) throws AdfsException
    {
        transaction(() -> {
            checkName(parts.getLast());
            Directory parent = parent(parts);
            if (parent.find(parts.getLast()) != null)
                throw new AdfsEntryExistsException("Entry already exists");
            int address = allocate(format.directoryFormat.size(), true);
            Directory child = new Directory(
                    parts.getLast(),
                    format.big() ? "" : parts.getLast(),
                    parent.discAddress(),
                    address,
                    List.of(),
                    0,
                    parent.signature(),
                    format.big() ? 2048 : 0);
            writeObject(address, format.directoryFormat.serialize(child));
            ArrayList<DirectoryEntry> entries = new ArrayList<>(parent.entries());
            entries.add(new DirectoryEntry(
                    parts.getLast(),
                    0,
                    0,
                    format.directoryFormat.size(),
                    address,
                    0,
                    Access.WR | Access.PR,
                    true));
            persist(parent.withEntries(sorted(entries)), parts.subList(0, parts.size() - 1));
        });
    }

    void remove(List<String> parts, boolean directory) throws AdfsException
    {
        transaction(() -> {
            Directory parent = parent(parts);
            DirectoryEntry entry = entry(parts);
            if (entry.directory() != directory)
                throw new AdfsPathException("Wrong object type");
            if ((entry.access() & Access.L) != 0)
                throw new AdfsFileLockedException("Object is locked");
            if (directory && !readDirectory(entry.discAddress()).entries().isEmpty())
                throw new AdfsDirectoryNotEmptyException("Directory is not empty");
            release(parent, entry);
            ArrayList<DirectoryEntry> entries = new ArrayList<>(parent.entries());
            entries.remove(entry);
            persist(parent.withEntries(entries), parts.subList(0, parts.size() - 1));
        });
    }

    void setMetadata(List<String> parts, AcornMeta meta) throws AdfsException
    {
        transaction(() -> {
            Directory parent = parent(parts);
            DirectoryEntry old = entry(parts);
            DirectoryEntry replacement = new DirectoryEntry(
                    old.name(),
                    meta.loadAddress(),
                    meta.executionAddress(),
                    old.length(),
                    old.discAddress(),
                    old.sequence(),
                    meta.access() == null ? old.access() : meta.access(),
                    old.directory());
            persist(
                    parent.withEntries(parent
                            .entries()
                            .stream()
                            .map(e -> e == old ?
                                    replacement :
                                    DirectoryFormat
                                            .nameKey(e.name())
                                            .equals(DirectoryFormat.nameKey(old.name())) ?
                                            replacement :
                                            e)
                            .toList()), parts.subList(0, parts.size() - 1));
        });
    }

    void setDirectoryTitle(List<String> parts, String title) throws AdfsException
    {
        format.directoryFormat.validateTitle(title);
        transaction(() -> {
            Directory dir = directory(parts);
            persist(
                    new Directory(
                            dir.name(),
                            title,
                            dir.parentAddress(),
                            dir.discAddress(),
                            dir.entries(),
                            dir.sequence(),
                            dir.signature(),
                            dir.bigSize()), parts);
        });
    }

    void rename(List<String> source, List<String> target) throws AdfsException
    {
        if (source.isEmpty() || target.isEmpty())
            throw new AdfsPathException("Cannot rename root");
        transaction(() -> {
            checkName(target.getLast());
            DirectoryEntry entry = entry(source);
            if (entry.directory() && target.size() > source.size() &&
                    keys(target.subList(0, source.size())).equals(keys(source)))
                throw new AdfsPathException("Cannot move a directory inside itself");
            Directory from = parent(source), to = parent(target);
            boolean same = from.discAddress() == to.discAddress();
            DirectoryEntry collision = to.find(target.getLast());
            if (collision != null)
                throw new AdfsEntryExistsException("Target exists");
            ArrayList<DirectoryEntry> sourceEntries = new ArrayList<>(from.entries());
            sourceEntries.removeIf(e -> DirectoryFormat
                    .nameKey(e.name())
                    .equals(DirectoryFormat.nameKey(entry.name())));
            if (same)
            {
                sourceEntries.add(entry.withName(target.getLast()));
                persist(
                        from.withEntries(sorted(sourceEntries)),
                        source.subList(0, source.size() - 1));
            } else
            {
                /* A shared fragment cannot follow a file into another sibling group. */
                DirectoryEntry moved = entry.withName(target.getLast());
                if (newMap != null && (entry.discAddress() & 255) != 0 && !entry.directory())
                {
                    int address = allocateFile(to, readObject(entry.discAddress(), entry.length()));
                    moved = moved.withAddress(address, moved.length());
                    release(from, entry);
                }
                ArrayList<DirectoryEntry> targetEntries = new ArrayList<>(to.entries());
                targetEntries.add(moved);
                persist(from.withEntries(sourceEntries), source.subList(0, source.size() - 1));
                persist(
                        to.withEntries(sorted(targetEntries)),
                        target.subList(0, target.size() - 1));
            }
        });
    }

    /* Validation walks each reachable directory once, reporting repeated addresses as cycles. */
    private int directoryScore() throws AdfsException
    {
        ArrayList<ValidationFinding> errors = new ArrayList<>();
        int count = walk(rootAddress, "$", new HashSet<>(), errors, false);
        return count + (errors.isEmpty() ? 1000000 : 0);
    }

    private int walk(
            int address,
            String path,
            Set<Integer> seen,
            List<ValidationFinding> errors,
            boolean findings)
    {
        if (!seen.add(address))
        {
            errors.add(new ValidationFinding(path, "Directory cycle or repeated address", null));
            return 0;
        }
        Directory dir;
        try
        {
            dir = readDirectory(address);
        } catch (AdfsException | IllegalArgumentException e)
        {
            errors.add(new ValidationFinding(path, e.getMessage(), null));
            return 0;
        }
        int count = 1;
        if (findings && format.directoryFormat == DirectoryFormat.OLD)
        {
            try
            {
                SectorsView bytes = SectorsView.wrap(readObject(address, 1280));
                int stored = bytes.get(1279), actual = DirectoryFormat.OLD.checksum(bytes);
                if (stored != 0 && stored != actual)
                    errors.add(new ValidationFinding(
                            path, "Stale old-directory check byte", () -> transaction(() -> {
                        byte[] raw = readObject(address, 1280);
                        raw[1279] = (byte) DirectoryFormat.OLD.checksum(SectorsView.wrap(raw));
                        writeObject(address, raw);
                    })));
            } catch (AdfsException e)
            {
                errors.add(new ValidationFinding(path, e.getMessage(), null));
            }
        }
        for (DirectoryEntry e : dir.entries())
        {
            String child = path + "." + e.name();
            if (findings && newMap == null && e.length() > 0 &&
                    (long) e.discAddress() * 256 + e.length() > storage.size())
            {
                int held = Math.max(0, storage.size() - e.discAddress() * 256);
                boolean remove = e.directory() || held == 0;
                String description = remove ?
                        "remove " + child :
                        "truncate " + child + " from " + e.length() + " to " + held + " bytes";
                errors.add(new ValidationFinding(
                        child,
                        "Object extends past the end of the image",
                        new org.rob_smallshire.oaknut.filesystem.Fix(
                                description, () -> transaction(() -> {
                            Directory current = readDirectory(address);
                            ArrayList<DirectoryEntry> repaired = new ArrayList<>();
                            for (DirectoryEntry entry : current.entries())
                            {
                                if (!entry.name().equals(e.name()))
                                    repaired.add(entry);
                                else if (!remove)
                                    repaired.add(entry.withAddress(entry.discAddress(), held));
                            }
                            writeObject(
                                    address,
                                    format.directoryFormat.serialize(current.withEntries(repaired)));
                        }))));
                continue;
            }
            if (e.directory())
                count += walk(e.discAddress(), child, seen, errors, findings);
            else if (findings)
            {
                try
                {
                    readObject(e.discAddress(), e.length());
                } catch (AdfsException ex)
                {
                    errors.add(new ValidationFinding(child, ex.getMessage(), null));
                }
            }
        }
        return count;
    }

    public ImmutableList<ValidationFinding> validate() throws AdfsException
    {
        requireOpen();
        ArrayList<ValidationFinding> result = new ArrayList<>();
        for (String message : newMap == null ? oldMap.validate() : newMap.validate())
            result.add(new ValidationFinding("$", message, null));
        walk(rootAddress, "$", new HashSet<>(), result, true);
        if (newMap == null && oldMap.totalSize() > storage.size() &&
                result.stream().anyMatch(f -> f.message().contains("past the end of the image")))
            result.add(new ValidationFinding(
                    "Disc size",
                    "The free-space map gives " + oldMap.totalSectors() +
                            " sectors, but the image holds " + storage.size() / 256,
                    new org.rob_smallshire.oaknut.filesystem.Fix(
                            "set the disc size to " + storage.size() / 256 + " sectors",
                            () -> transaction(() -> oldMap.shrinkTo(storage.size() / 256)))));
        return ImmutableList.copyOf(result);
    }

    public void exportAll(Path target) throws IOException
    {
        exportTree(root(), target);
    }

    private void exportTree(AdfsPath directory, Path target) throws IOException
    {
        Files.createDirectories(target);
        for (AdfsPath child : directory.list())
        {
            Path destination = target.resolve(child.name());
            if (child.isDirectory())
                exportTree(child, destination);
            else
                child.exportFile(destination);
        }
    }

    public int compact() throws AdfsException
    {
        if (newMap != null || format == AdfsFormat.D)
            throw new AdfsException("Compaction supports old directories only, as upstream");
        requireWritable();
        Adfs replacement = create(format, title(), bootOption(), oldMap.discId());
        replacement.oldMap.shrinkTo(Math.min(oldMap.totalSectors(), storage.size() / 256));
        int count = copyTree(root(), replacement.root());
        transaction(() -> {
            new SectorsView(storage, 0, storage.size()).write(
                    0,
                    java.util.Arrays.copyOf(replacement.toByteArray(), storage.size()));
            loadMaps();
        });
        return count;
    }

    @FunctionalInterface
    interface Mutation
    {
        void run() throws AdfsException;
    }
}
