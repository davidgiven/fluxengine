package org.rob_smallshire.oaknut.dfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.Surface;
import org.rob_smallshire.oaknut.filesystem.Fix;
import org.rob_smallshire.oaknut.filesystem.ValidationFinding;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * The two DFS catalogues share the entry codec. A directory character is stored in the
 * filename itself; it never denotes a navigable directory or a multipart path. */
public abstract class Catalogue
{
    protected final Surface surface;
    private final boolean watford;

    protected Catalogue(Surface surface, boolean watford)
    {
        this.surface = surface;
        this.watford = watford;
    }

    static int unsigned(byte value)
    {
        return value & 255;
    }

    private static int word(byte[] bytes, int offset)
    {
        return unsigned(bytes[offset]) | unsigned(bytes[offset + 1]) << 8;
    }

    private static void word(byte[] bytes, int offset, long value)
    {
        bytes[offset] = (byte) value;
        bytes[offset + 1] = (byte) (value >>> 8);
    }

    public static long expandHostAddress(long value)
    {
        return (value & 0x30000) == 0x30000 ? 0xffff0000L | (value & 0xffff) : value;
    }

    /* Bare names use $. Explicit directory characters remain part of the returned flat name.
     * Reading existing catalogues permits non-command directory bytes such as '&'. */
    public static String filename(String name, boolean verbatim)
    {
        String full = name.length() >= 2 && name.charAt(1) == '.' ? name : "$." + name;
        char directory = full.charAt(0);
        if (directory > 127 || directory == '.' || directory == ':' ||
                (!verbatim && directory != '$' && (Character.toUpperCase(directory) < 'A' ||
                        Character.toUpperCase(directory) > 'Z')))
            throw new IllegalArgumentException("Invalid DFS directory character: " + directory);
        String leaf = full.substring(2);
        if (leaf.isEmpty() || leaf.length() > 7 || leaf.endsWith(" ") ||
                leaf.chars().anyMatch(c -> c > 127 || c == '.' || c == ':'))
            throw new IllegalArgumentException("Invalid DFS filename: " + name);
        return full;
    }

    static String key(String name)
    {
        return filename(name, true).toUpperCase(Locale.ROOT);
    }

    private static String text(byte[] bytes, int start, int length, boolean spacesOnly)
    {
        String value = new String(bytes, start, length, StandardCharsets.ISO_8859_1);
        int end = value.length();
        while (end > 0 &&
                (value.charAt(end - 1) == ' ' || (!spacesOnly && value.charAt(end - 1) == 0)))
            end--;
        return value.substring(0, end);
    }

    private static void checkSectors(int sectors)
    {
        if (sectors < 1 || sectors > 1023)
            throw new IllegalArgumentException("DFS sector count outside 1..1023");
    }

    private static void checkBoot(int option)
    {
        if (option < 0 || option > 3)
            throw new IllegalArgumentException("DFS boot option outside 0..3");
    }

    public boolean watford()
    {
        return watford;
    }

    public int catalogueSectors()
    {
        return watford ? 4 : 2;
    }

    public int maxFiles()
    {
        return watford ? 62 : 31;
    }

    public int heldSectors()
    {
        int count = 0;
        while (count < surface.spec().sectors())
        {
            try
            {
                surface.sectorRange(count, 1);
                count++;
            } catch (IndexOutOfBoundsException e)
            {
                break;
            }
        }
        return count;
    }

    /* Copy catalogue bytes before editing so validation failures do not leave partial entries. */
    protected byte[] read(int first, int count) throws IOException
    {
        try
        {
            return surface.sectorRange(first, count).toByteArray();
        } catch (UncheckedIOException e)
        {
            throw e.getCause();
        } catch (IndexOutOfBoundsException e)
        {
            throw new DfsFormatException("Catalogue or file extends beyond the image");
        }
    }

    protected void write(int first, byte[] bytes) throws IOException
    {
        try
        {
            surface.sectorRange(first, bytes.length / 256).write(0, bytes);
        } catch (UncheckedIOException e)
        {
            throw e.getCause();
        }
    }

    public void validateTitle(String title)
    {
        if (title.length() > (watford ? 10 : 12) ||
                title.chars().anyMatch(c -> c > 127 || (c > 0 && c < 32)))
            throw new IllegalArgumentException("Invalid DFS title: " + title);
    }

    private String title(byte[] bytes)
    {
        byte[] title = new byte[watford ? 10 : 12];
        System.arraycopy(bytes, 0, title, 0, 8);
        System.arraycopy(bytes, 256, title, 8, title.length - 8);
        return text(title, 0, title.length, false);
    }

    public Assessment assess() throws IOException
    {
        if (heldSectors() < catalogueSectors())
            return Assessment.mismatch("Image too small for catalogue");
        byte[] bytes = read(0, catalogueSectors());
        boolean marker = false;
        if (heldSectors() >= 4)
        {
            byte[] extra = watford ? bytes : read(2, 2);
            int start = watford ? 512 : 0;
            marker = true;
            for (int i = 0; i < 8; i++)
                marker &= unsigned(extra[start + i]) == 0xaa;
            if (!watford && (marker ||
                    (word(extra, 256) == 0 && word(extra, 258) == 0 && (extra[261] & 7) == 0 &&
                            extra[262] == bytes[262] && extra[263] == bytes[263])))
                return Assessment.mismatch("Watford DFS second catalogue section");
        }
        if (watford && !marker)
            return Assessment.mismatch("No Watford marker: expected eight AA bytes in sector 2");
        boolean control = false;
        for (int offset : new int[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 256, 257, 258, 259})
        {
            int value = unsigned(bytes[offset]);
            if (value > 127 || (watford && value > 0 && value < 32))
                return Assessment.mismatch("Invalid title character at " + offset);
            control |= value > 0 && value < 32;
        }
        if ((bytes[261] & 7) != 0)
            return Assessment.mismatch("File count byte is not a multiple of eight");
        if ((bytes[262] & (watford ? 0xcc : 0xc0)) != 0)
            return Assessment.mismatch("Reserved boot/sector-count bits are set");
        if (watford)
        {
            if (word(bytes, 768) != 0 || word(bytes, 770) != 0 || (bytes[773] & 7) != 0)
                return Assessment.mismatch("Invalid second catalogue header");
            if (bytes[774] != bytes[262] || bytes[775] != bytes[263])
                return Assessment.mismatch("Catalogue sections disagree on boot option or size");
            int count = declaredSectors(bytes);
            if (count < 4 || count % 10 != 0)
                return Assessment.mismatch("Implausible Watford sector count: " + count);
            for (int section = 0; section < 2; section++)
                for (int i = 0; i < unsigned(bytes[section * 512 + 261]) / 8; i++)
                    if ((bytes[section * 512 + 8 + i * 8 + 5] & 0x80) != 0 ||
                            (bytes[section * 512 + 8 + i * 8 + 6] & 0x80) != 0)
                        return Assessment.mismatch("Unsupported Watford >256K extension");
        } else if (control || countImplausible(bytes))
        {
            List<DfsFileEntry> files = entries(bytes);
            if (files.isEmpty())
                return Assessment.mismatch("Suspect metadata and no files to corroborate " +
                        "catalogue");
            for (DfsFileEntry file : files)
                if (file.startSector() < 2 ||
                        file.startSector() + file.sectorsRequired() > heldSectors())
                    return Assessment.mismatch(
                            "Suspect metadata and file outside image: " + file.name());
        }
        return new Assessment(
                true,
                ImmutableList.of(
                        "Well-formed " + (watford ? "Watford" : "Acorn") + " DFS catalogue"),
                null);
    }

    private int declaredSectors(byte[] bytes)
    {
        return ((bytes[262] & 3) << 8) | unsigned(bytes[263]);
    }

    private boolean countImplausible(byte[] bytes)
    {
        int count = declaredSectors(bytes);
        return (bytes[262] & 12) != 0 || count < 10 || count % 10 != 0;
    }

    private String countDefect(byte[] bytes, List<DfsFileEntry> files)
    {
        int count = declaredSectors(bytes);
        if (countImplausible(bytes))
            return "Malformed sector count: " + count;
        int extent = catalogueSectors();
        for (DfsFileEntry file : files)
            extent = Math.max(extent, file.startSector() + file.sectorsRequired());
        return count < extent && count < heldSectors() ?
                "Files run past declared sector count: " + count :
                null;
    }

    private ImmutableList<DfsFileEntry> entries(byte[] bytes) throws DfsFormatException
    {
        ImmutableList.Builder<DfsFileEntry> result = ImmutableList.builder();
        for (int section = 0; section < (watford ? 2 : 1); section++)
        {
            int base = section * 512;
            if ((bytes[base + 261] & 7) != 0)
                throw new DfsFormatException("Invalid file count byte");
            for (int i = 0; i < unsigned(bytes[base + 261]) / 8; i++)
            {
                int offset = base + 8 + i * 8;
                int flags = unsigned(bytes[offset + 7]);
                int extra = unsigned(bytes[offset + 262]);
                String name = (char) (flags & 127) + "." + text(bytes, offset, 7, true);
                result.add(new DfsFileEntry(
                        name,
                        (flags & 128) != 0,
                        expandHostAddress(word(bytes, offset + 256) | (extra & 12) << 14),
                        expandHostAddress(word(bytes, offset + 258) | (extra & 192) << 10),
                        word(bytes, offset + 260) | (extra & 48) << 12,
                        unsigned(bytes[offset + 263]) | (extra & 3) << 8));
            }
        }
        return result.build();
    }

    public ImmutableList<DfsFileEntry> files() throws IOException
    {
        return entries(read(0, catalogueSectors()));
    }

    public DfsDiscInfo info() throws IOException
    {
        byte[] bytes = read(0, catalogueSectors());
        ImmutableList<DfsFileEntry> files = entries(bytes);
        int total = !watford && countDefect(bytes, files) != null ?
                heldSectors() :
                declaredSectors(bytes);
        return new DfsDiscInfo(
                title(bytes),
                unsigned(bytes[260]),
                files.size(),
                total,
                (bytes[262] >>> 4) & 3);
    }

    public DfsFileEntry find(String name) throws IOException
    {
        String target = key(name);
        for (DfsFileEntry file : files())
            if (key(file.name()).equals(target))
                return file;
        return null;
    }

    public DfsFileEntry stat(String name) throws IOException
    {
        DfsFileEntry entry = find(name);
        if (entry == null)
            throw new NoSuchFileException(name);
        return entry;
    }

    public void initialise(int sectors, String title, int bootOption) throws IOException
    {
        validateTitle(title);
        checkSectors(sectors);
        checkBoot(bootOption);
        byte[] bytes = new byte[catalogueSectors() * 256];
        putTitle(bytes, title);
        if (watford)
            Arrays.fill(bytes, 512, 520, (byte) 0xaa);
        for (int section = 0; section < (watford ? 2 : 1); section++)
        {
            bytes[section * 512 + 262] = (byte) ((sectors >>> 8) | bootOption << 4);
            bytes[section * 512 + 263] = (byte) sectors;
        }
        write(0, bytes);
    }

    private void putTitle(byte[] bytes, String value)
    {
        byte[] padded = new byte[watford ? 10 : 12];
        Arrays.fill(padded, (byte) ' ');
        byte[] title = value.getBytes(StandardCharsets.ISO_8859_1);
        System.arraycopy(title, 0, padded, 0, title.length);
        System.arraycopy(padded, 0, bytes, 0, 8);
        System.arraycopy(padded, 8, bytes, 256, padded.length - 8);
    }

    /* Acorn entries are descending by physical start sector. Watford retains section order.
     * Each section's master sequence number is independent, never copied from section one. */
    public void replaceEntries(List<DfsFileEntry> files) throws IOException
    {
        if (files.size() > maxFiles())
            throw new DfsCatalogueFullException("Catalogue full: " + maxFiles() + " files");
        Set<String> names = new HashSet<>();
        for (DfsFileEntry file : files)
        {
            if (!names.add(key(file.name())))
                throw new FileAlreadyExistsException(file.name());
            if (file.length() < 0 || file.length() > 0x3ffff ||
                    file.startSector() < catalogueSectors() || file.startSector() > 1023 ||
                    file.loadAddress() < 0 || file.loadAddress() > 0xffffffffL ||
                    file.executionAddress() < 0 || file.executionAddress() > 0xffffffffL)
                throw new IllegalArgumentException("Invalid DFS entry: " + file.name());
        }
        byte[] old = read(0, catalogueSectors());
        DfsDiscInfo info = info();
        byte[] bytes = new byte[old.length];
        putTitle(bytes, info.title());
        if (watford)
            Arrays.fill(bytes, 512, 520, (byte) 0xaa);
        ArrayList<DfsFileEntry> ordered = new ArrayList<>(files);
        if (!watford)
            ordered.sort(Comparator.comparingInt(DfsFileEntry::startSector).reversed());
        for (int section = 0; section < (watford ? 2 : 1); section++)
        {
            int base = section * 512;
            int count = Math.max(0, Math.min(31, files.size() - section * 31));
            bytes[base + 260] = old[base + 260];
            bytes[base + 261] = (byte) (count * 8);
            bytes[base + 262] = (byte) ((info.totalSectors() >>> 8) | info.bootOption() << 4);
            bytes[base + 263] = (byte) info.totalSectors();
            for (int i = 0; i < count; i++)
            {
                DfsFileEntry file = ordered.get(section * 31 + i);
                String name = filename(file.name(), true);
                int offset = base + 8 + i * 8;
                Arrays.fill(bytes, offset, offset + 7, (byte) ' ');
                byte[] leaf = name.substring(2).getBytes(StandardCharsets.ISO_8859_1);
                System.arraycopy(leaf, 0, bytes, offset, leaf.length);
                bytes[offset + 7] = (byte) (name.charAt(0) | (file.locked() ? 128 : 0));
                word(bytes, offset + 256, file.loadAddress());
                word(bytes, offset + 258, file.executionAddress());
                word(bytes, offset + 260, file.length());
                bytes[offset + 262] = (byte) ((file.startSector() >>> 8) |
                        ((file.loadAddress() >>> 16) & 3) << 2 | ((file.length() >>> 16) & 3) << 4 |
                        ((file.executionAddress() >>> 16) & 3) << 6);
                bytes[offset + 263] = (byte) file.startSector();
            }
            /* A Watford insertion in section two must not advance section one's sequence.
             * Compare each rebuilt section independently before changing its sequence byte. */
            if (!watford || !Arrays.equals(old, base, base + 512, bytes, base, base + 512))
                bytes[base + 260]++;
        }
        write(0, bytes);
    }

    public void setTitle(String title) throws IOException
    {
        validateTitle(title);
        byte[] bytes = read(0, catalogueSectors());
        putTitle(bytes, title);
        bytes[260]++;
        write(0, bytes);
    }

    public void setBootOption(int option) throws IOException
    {
        checkBoot(option);
        byte[] bytes = read(0, catalogueSectors());
        for (int section = 0; section < (watford ? 2 : 1); section++)
            bytes[section * 512 + 262] = (byte) ((bytes[section * 512 + 262] & 3) | option << 4);
        bytes[260]++;
        write(0, bytes);
    }

    public void setTotalSectors(int sectors) throws IOException
    {
        checkSectors(sectors);
        byte[] bytes = read(0, catalogueSectors());
        for (int section = 0; section < (watford ? 2 : 1); section++)
        {
            bytes[section * 512 + 262] = (byte) ((bytes[section * 512 + 262] & 48) | sectors >>> 8);
            bytes[section * 512 + 263] = (byte) sectors;
        }
        bytes[260]++;
        write(0, bytes);
    }

    public void rename(String source, String destination) throws IOException
    {
        String name = filename(destination, false);
        DfsFileEntry original = stat(source);
        if (find(name) != null)
            throw new FileAlreadyExistsException(name);
        ArrayList<DfsFileEntry> files = new ArrayList<>(files());
        files.set(files.indexOf(original), original.withName(name));
        replaceEntries(files);
    }

    public void setMetadata(String name, org.rob_smallshire.oaknut.file.AcornMeta metadata)
            throws IOException
    {
        DfsFileEntry original = stat(name);
        ArrayList<DfsFileEntry> files = new ArrayList<>(files());
        files.set(files.indexOf(original), original.withMetadata(metadata));
        replaceEntries(files);
    }

    public void delete(String name) throws IOException
    {
        DfsFileEntry original = stat(name);
        if (original.locked())
            throw new DfsFileLockedException(name);
        ArrayList<DfsFileEntry> files = new ArrayList<>(files());
        files.remove(original);
        replaceEntries(files);
    }

    /* Validation is read-only. Repairs are explicit actions attached to individual findings. */
    public ImmutableList<ValidationFinding> validate() throws IOException
    {
        ImmutableList.Builder<ValidationFinding> findings = ImmutableList.builder();
        byte[] bytes = read(0, catalogueSectors());
        List<DfsFileEntry> files = entries(bytes);
        String defect = countDefect(bytes, files);
        if (defect != null)
            findings.add(new ValidationFinding(
                    "",
                    defect,
                    new Fix(
                            "Set sector count to image size",
                            () -> setTotalSectors(heldSectors()))));
        if (watford && (bytes[262] != bytes[774] || bytes[263] != bytes[775]))
            findings.add(new ValidationFinding(
                    "",
                    "Catalogue sections disagree on boot option or size",
                    null));
        Set<String> names = new HashSet<>();
        String[] used = new String[surface.spec().sectors()];
        Arrays.fill(used, 0, catalogueSectors(), "catalogue");
        for (DfsFileEntry file : files)
        {
            if (!names.add(file.name().toUpperCase(Locale.ROOT)))
                findings.add(new ValidationFinding(file.name(), "Duplicate filename", null));
            int end = file.startSector() + file.sectorsRequired();
            if (file.startSector() < catalogueSectors() || end > heldSectors())
                findings.add(new ValidationFinding(
                        file.name(),
                        "File extends outside data area",
                        null));
            for (int sector = file.startSector(); sector < Math.min(end, used.length); sector++)
            {
                if (used[sector] != null)
                    findings.add(new ValidationFinding(
                            file.name(),
                            "Sector shared with " + used[sector],
                            null));
                used[sector] = file.name();
            }
        }
        return findings.build();
    }

    /* Recognition accepts truncated captures and corroborates suspect Acorn metadata with
     * actual file extents. Watford's extension bits for discs above 256K are unsupported. */
    public record Assessment(boolean matches, ImmutableList<String> evidence, String reason)
    {
        static Assessment mismatch(String reason)
        {
            return new Assessment(false, ImmutableList.of(), reason);
        }
    }
}
