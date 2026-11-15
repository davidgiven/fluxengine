package org.rob_smallshire.oaknut.dfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.BufferStorage;
import org.rob_smallshire.oaknut.discimage.DiscImage;
import org.rob_smallshire.oaknut.discimage.Storage;
import org.rob_smallshire.oaknut.discimage.Surface;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.file.TextIo;
import org.rob_smallshire.oaknut.file.HostBridge;
import org.rob_smallshire.oaknut.file.MetaFormat;
import org.rob_smallshire.oaknut.filesystem.ValidationFinding;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Standalone DFS image access. A handle selects one side; flat filenames include the DFS
 * directory letter. Storage is borrowed unless opened through fromFile/createFile. */
public final class Dfs implements AutoCloseable
{
    private final Storage storage;
    private final DfsFormat format;
    private final int side;
    private final CataloguedSurface files;
    private final boolean rollbackEnabled;
    private final FileChannel ownedChannel;
    private boolean closed;

    private Dfs(Storage storage, DfsFormat format, int side, boolean rollbackEnabled,
            FileChannel channel, boolean recognise) throws IOException
    {
        if (side < 0 || side >= format.sides)
            throw new IllegalArgumentException("Invalid DFS side: " + side);
        this.storage = storage;
        this.format = format;
        this.side = side;
        this.rollbackEnabled = rollbackEnabled;
        ownedChannel = channel;
        Surface surface = new DiscImage(storage, format.surfaces()).surface(side);
        Catalogue catalogue = format.watford ? new WatfordDfsCatalogue(surface) : new AcornDfsCatalogue(surface);
        if (recognise)
        {
            Catalogue.Assessment assessment = catalogue.assess();
            if (!assessment.matches())
                throw new DfsFormatException(assessment.reason());
        }
        files = new CataloguedSurface(catalogue);
    }

    public static Dfs fromStorage(Storage storage, DfsFormat format, int side, boolean rollbackEnabled)
            throws IOException
    {
        return new Dfs(storage, format, side, rollbackEnabled, null, true);
    }

    public static Dfs fromStorage(Storage storage, DfsFormat format, boolean rollbackEnabled) throws IOException
    {
        return fromStorage(storage, format, 0, rollbackEnabled);
    }

    /* Buffer captures may end mid-sector. Pad only this in-memory entry point, preserving
     * upstream's trimmed-image support without hiding missing reads from external Storage. */
    public static Dfs fromBuffer(ByteBuffer buffer, DfsFormat format, int side) throws IOException
    {
        if (buffer.remaining() < 512)
            throw new DfsFormatException("Image too small for a DFS catalogue");
        ByteBuffer view = buffer.slice();
        if (view.remaining() < format.size())
        {
            ByteBuffer padded = ByteBuffer.allocate(format.size());
            padded.put(view).clear();
            view = buffer.isReadOnly() ? padded.asReadOnlyBuffer() : padded;
        } else
            view.limit(format.size());
        return fromStorage(new BufferStorage(view), format, side, true);
    }

    public static Dfs fromBuffer(ByteBuffer buffer, DfsFormat format) throws IOException
    {
        return fromBuffer(buffer, format, 0);
    }

    public static Dfs create(DfsFormat format, String title, int bootOption) throws IOException
    {
        Storage storage = new BufferStorage(ByteBuffer.allocate(format.size()));
        DiscImage image = new DiscImage(storage, format.surfaces());
        for (int side = 0; side < format.sides; side++)
        {
            Catalogue catalogue = format.watford ? new WatfordDfsCatalogue(image.surface(side)) :
                    new AcornDfsCatalogue(image.surface(side));
            catalogue.initialise(format.tracks * 10, side == 0 ? title : "", bootOption);
        }
        return fromStorage(storage, format, true);
    }

    public static Dfs create(DfsFormat format, String title) throws IOException
    {
        return create(format, title, 0);
    }

    public static Dfs create(DfsFormat format) throws IOException
    {
        return create(format, "", 0);
    }

    /* File storage performs positional reads, zero-filling only a truncated file's tail.
     * Writes can extend the file. Borrowed devices instead enforce their own exact I/O rules. */
    private static final class FileStorage implements Storage
    {
        private final FileChannel channel;
        private final int size;
        private final boolean readOnly;

        FileStorage(FileChannel channel, int size, boolean readOnly)
        {
            this.channel = channel;
            this.size = size;
            this.readOnly = readOnly;
        }

        @Override
        public int size()
        {
            return size;
        }

        @Override
        public boolean isReadOnly()
        {
            return readOnly;
        }

        @Override
        public void read(int offset, byte[] destination, int destinationOffset, int length) throws IOException
        {
            java.util.Objects.checkFromIndexSize(offset, length, size);
            ByteBuffer target = ByteBuffer.wrap(destination, destinationOffset, length);
            int position = offset;
            while (target.hasRemaining())
            {
                int count = channel.read(target, position);
                if (count < 0)
                {
                    while (target.hasRemaining())
                        target.put((byte) 0);
                    break;
                }
                position += count;
            }
        }

        @Override
        public void write(int offset, byte[] source, int sourceOffset, int length) throws IOException
        {
            java.util.Objects.checkFromIndexSize(offset, length, size);
            if (readOnly)
                throw new DfsReadOnlyException("Read-only image");
            ByteBuffer bytes = ByteBuffer.wrap(source, sourceOffset, length);
            int position = offset;
            while (bytes.hasRemaining())
                position += channel.write(bytes, position);
        }

        @Override
        public void flush() throws IOException
        {
            if (!readOnly)
                channel.force(true);
        }
    }

    public static Dfs fromFile(Path path, DfsFormat format, int side, boolean readOnly) throws IOException
    {
        FileChannel channel = readOnly ? FileChannel.open(path, StandardOpenOption.READ) :
                FileChannel.open(path, StandardOpenOption.READ, StandardOpenOption.WRITE);
        try
        {
            if (channel.size() < 512)
                throw new DfsFormatException("Image too small for DFS");
            Storage storage = new FileStorage(channel, format.size(), readOnly);
            return new Dfs(storage, format, side, true, channel, true);
        } catch (IOException | RuntimeException e)
        {
            channel.close();
            throw e;
        }
    }

    public static Dfs fromFile(Path path, boolean readOnly) throws IOException
    {
        int size = Math.toIntExact(java.nio.file.Files.size(path));
        DfsFormat format = DfsFormat.detect(path.toString(), size);
        try (FileChannel channel = FileChannel.open(path, StandardOpenOption.READ))
        {
            Surface surface = new DiscImage(new FileStorage(channel, format.size(), true), format.surfaces()).surface(0);
            if (new WatfordDfsCatalogue(surface).assess().matches())
                format = format.withWatford(true);
        }
        return fromFile(path, format, 0, readOnly);
    }

    public static Dfs fromFile(Path path) throws IOException
    {
        return fromFile(path, false);
    }

    public static Dfs createFile(Path path, DfsFormat format, String title) throws IOException
    {
        try (Dfs image = create(format, title);
                FileChannel channel = FileChannel.open(path, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE))
        {
            ByteBuffer bytes = ByteBuffer.wrap(image.toByteArray());
            while (bytes.hasRemaining())
                channel.write(bytes);
        }
        return fromFile(path, format, 0, false);
    }

    public static Dfs createFile(Path path, String title) throws IOException
    {
        return createFile(path, DfsFormat.detect(path.toString(),
                path.toString().toLowerCase(java.util.Locale.ROOT).endsWith(".dsd") ? 409600 : 204800), title);
    }

    private void requireOpen() throws DfsClosedException
    {
        if (closed)
            throw new DfsClosedException("DFS image is closed");
    }

    @FunctionalInterface
    private interface Mutation<T>
    {
        T run() throws IOException;
    }

    /* External storage owners can disable snapshots and supply their own commit/revert.
     * On failure with snapshots enabled, restore the image before propagating the error. */
    private <T> T mutate(Mutation<T> mutation) throws IOException
    {
        requireOpen();
        if (storage.isReadOnly())
            throw new DfsReadOnlyException("DFS image is read-only");
        byte[] snapshot = rollbackEnabled ? toByteArray() : null;
        try
        {
            return mutation.run();
        } catch (IOException | RuntimeException e)
        {
            if (snapshot != null)
                try
                {
                    storage.write(0, snapshot, 0, snapshot.length);
                } catch (IOException | RuntimeException restore)
                {
                    e.addSuppressed(restore);
                }
            throw e;
        }
    }

    public DfsFormat format()
    {
        return format;
    }

    public int side()
    {
        return side;
    }

    public boolean closed()
    {
        return closed;
    }

    public boolean isReadOnly()
    {
        return storage.isReadOnly();
    }

    public DfsDiscInfo info() throws IOException
    {
        requireOpen();
        return files.catalogue().info();
    }

    public String title() throws IOException
    {
        return info().title();
    }

    public int bootOption() throws IOException
    {
        return info().bootOption();
    }

    public void setTitle(String value) throws IOException
    {
        mutate(() -> { files.catalogue().setTitle(value); return null; });
    }

    public void setBootOption(int value) throws IOException
    {
        mutate(() -> { files.catalogue().setBootOption(value); return null; });
    }

    public void setTotalSectors(int value) throws IOException
    {
        mutate(() -> { files.catalogue().setTotalSectors(value); return null; });
    }

    public ImmutableList<DfsFileEntry> files() throws IOException
    {
        requireOpen();
        return files.catalogue().files();
    }

    public DfsFileEntry stat(String name) throws IOException
    {
        requireOpen();
        return files.catalogue().stat(name);
    }

    public boolean exists(String name) throws IOException
    {
        requireOpen();
        return files.catalogue().find(name) != null;
    }

    public byte[] readFile(String name) throws IOException
    {
        requireOpen();
        return files.readFile(name);
    }

    public void writeFile(String name, byte[] data, AcornMeta metadata) throws IOException
    {
        mutate(() -> { files.writeFile(name, data, metadata); return null; });
    }

    /* Verbatim names preserve unusual stored directory bytes when copying archival discs. */
    public void writeFile(String name, byte[] data, AcornMeta metadata, boolean verbatimName) throws IOException
    {
        mutate(() -> { files.writeFile(name, data, metadata, verbatimName); return null; });
    }

    public void writeFile(String name, byte[] data) throws IOException
    {
        writeFile(name, data, new AcornMeta(0, 0, null));
    }

    public void deleteFile(String name) throws IOException
    {
        mutate(() -> { files.catalogue().delete(name); return null; });
    }

    public void touch(String name, boolean existOk) throws IOException
    {
        if (exists(name))
        {
            if (!existOk)
                throw new java.nio.file.FileAlreadyExistsException(name);
            return;
        }
        writeFile(name, new byte[0]);
    }

    public void copyFile(String source, String destination) throws IOException
    {
        if (exists(destination))
            throw new java.nio.file.FileAlreadyExistsException(destination);
        writeFile(destination, readFile(source), stat(source).metadata());
    }

    /* Host metadata handling is shared with ADFS; DFS interprets addresses as raw values,
     * rather than RISC OS file types. Only the canonical access lock bit is representable. */
    public Path exportFile(String name, Path target, MetaFormat format) throws IOException
    {
        return HostBridge.exportFile(target, stat(name).name(), readFile(name), stat(name).metadata(),
                format, 0, false);
    }

    public Path exportFile(String name, Path target) throws IOException
    {
        return exportFile(name, target, MetaFormat.INF_TRAD);
    }

    public void importFile(String name, Path source, List<MetaFormat> formats) throws IOException
    {
        HostBridge.Imported imported = HostBridge.importFile(source, formats);
        writeFile(name, imported.bytes(), imported.metadata());
    }

    public void importFile(String name, Path source) throws IOException
    {
        importFile(name, source, HostBridge.DEFAULT_IMPORT_FORMATS);
    }

    public static int expand(Path path, DfsFormat format) throws IOException
    {
        try (FileChannel channel = FileChannel.open(path, StandardOpenOption.READ, StandardOpenOption.WRITE))
        {
            long size = channel.size();
            if (size == 0 || size > format.size())
                throw new DfsFormatException("Image cannot be expanded to selected format");
            if (size < format.size())
                channel.write(ByteBuffer.wrap(new byte[1]), format.size() - 1);
            return (int) (format.size() - size);
        }
    }

    public void renameFile(String source, String destination) throws IOException
    {
        mutate(() -> { files.catalogue().rename(source, destination); return null; });
    }

    public void setMetadata(String name, AcornMeta metadata) throws IOException
    {
        mutate(() -> { files.catalogue().setMetadata(name, metadata); return null; });
    }

    public void setLoadAddress(String name, long address) throws IOException
    {
        AcornMeta previous = stat(name).metadata();
        setMetadata(name, new AcornMeta(address, previous.executionAddress(), previous.access()));
    }

    public void setExecutionAddress(String name, long address) throws IOException
    {
        AcornMeta previous = stat(name).metadata();
        setMetadata(name, new AcornMeta(previous.loadAddress(), address, previous.access()));
    }

    public void setLocked(String name, boolean locked) throws IOException
    {
        AcornMeta previous = stat(name).metadata();
        setMetadata(name, new AcornMeta(previous.loadAddress(), previous.executionAddress(),
                locked ? Access.L | Access.R : Access.R | Access.W));
    }

    public String readText(String name) throws IOException
    {
        return TextIo.decode(readFile(name), true);
    }

    public void writeText(String name, String text) throws IOException
    {
        writeFile(name, TextIo.encode(text, "\r"));
    }

    public int freeSectors() throws IOException
    {
        requireOpen();
        return files.freeSectors();
    }

    public ImmutableList<CataloguedSurface.FreeRegion> freeMap() throws IOException
    {
        requireOpen();
        return files.freeMap();
    }

    public int compact(List<String> order) throws IOException
    {
        return mutate(() -> files.compact(order));
    }

    public int compact() throws IOException
    {
        return compact(List.of());
    }

    public ImmutableList<ValidationFinding> validate() throws IOException
    {
        requireOpen();
        return files.catalogue().validate();
    }

    public byte[] toByteArray() throws IOException
    {
        requireOpen();
        byte[] bytes = new byte[Math.min(format.size(), storage.size())];
        storage.read(0, bytes, 0, bytes.length);
        return bytes;
    }

    public void flush() throws IOException
    {
        requireOpen();
        storage.flush();
    }

    @Override
    public void close() throws IOException
    {
        if (closed)
            return;
        try
        {
            storage.flush();
        } finally
        {
            closed = true;
            if (ownedChannel != null)
                ownedChannel.close();
        }
    }
}
