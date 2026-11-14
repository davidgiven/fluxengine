package org.rob_smallshire.oaknut.adfs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.nio.charset.StandardCharsets;
import org.rob_smallshire.oaknut.discimage.SectorsView;
import org.rob_smallshire.oaknut.file.AcornEncoding;
import org.rob_smallshire.oaknut.file.Access;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Old, New and Big directory codecs. Serialization is staged before touching an image. */
public enum DirectoryFormat
{
    OLD(1280, 47),
    NEW(2048, 77),
    BIG(2048, Integer.MAX_VALUE);

    private final int size;
    private final int maxEntries;
    DirectoryFormat(int size, int maxEntries)
    {
        this.size = size;
        this.maxEntries = maxEntries;
    }
    public int size()
    { return size; }
    public int maxEntries()
    { return maxEntries; }

    /* Acorn folds ASCII letters, leaving extended Latin-1 characters unchanged. */
    public static String nameKey(String name)
    { return name.toUpperCase(java.util.Locale.ROOT); }

    public void validateName(String name)
    {
        AcornEncoding.encode(name, this != BIG);
        if (name.isEmpty() || name.length() > (this == BIG ? 255 : 10) ||
            name.chars().anyMatch(c -> ".:\r".indexOf(c) >= 0))
            throw new IllegalArgumentException("Invalid ADFS name: " + name);
    }

    public void validateTitle(String title)
    {
        if (this == BIG)
            throw new UnsupportedOperationException("Big directories have no titles");
        AcornEncoding.checkTitle(title, 19, this == OLD);
    }

    private static int wordAlign(int n)
    { return (n + 3) & ~3; }
    public int requiredSize(Directory dir, int sectorSize)
    {
        if (this != BIG)
            return size;
        int names = dir.entries().stream().mapToInt(e -> e.name().length() + 1).sum();
        int needed = wordAlign(28 + dir.name().length() + 1) + dir.entries().size() * 28 +
                     wordAlign(names) + 8;
        return Math.max(2048, (needed + sectorSize - 1) / sectorSize * sectorSize);
    }

    private static String text(SectorsView b, int offset, int n, boolean old)
    { return AcornEncoding.decode(b.read(offset, n), old); }

    private static void field(SectorsView b, int offset, int n, String text, boolean ascii)
    {
        byte[] encoded = AcornEncoding.encode(text, ascii);
        byte[] padded = new byte[n];
        Arrays.fill(padded, (byte)13);
        System.arraycopy(encoded, 0, padded, 0, Math.min(n, encoded.length));
        b.write(offset, padded);
    }

    public Directory parse(SectorsView b, int address) throws AdfsDirectoryException
    {
        try
        {
            return parseChecked(b, address);
        }
        catch (IndexOutOfBoundsException | IllegalArgumentException | ArithmeticException e)
        {
            throw new AdfsDirectoryException("Directory extends beyond its allocated block");
        }
    }

    private Directory parseChecked(SectorsView b, int address) throws AdfsDirectoryException
    {
        int sequence = b.get(0);
        String signature = new String(b.read(this == BIG ? 4 : 1, 4), StandardCharsets.US_ASCII);
        if (!(this == BIG ? signature.equals("SBPr")
                          : signature.equals("Hugo") || signature.equals("Nick")))
            throw new AdfsDirectoryException("Invalid directory signature: " + signature);
        int actual = this == BIG ? Math.toIntExact(b.readLe(12, 4)) : size;
        if (actual > b.size() || actual < size)
            throw new AdfsDirectoryException("Invalid directory size " + actual);
        int tail = actual - (this == OLD ? 53 : this == NEW ? 41 : 8);
        int seqOffset = tail + (this == OLD ? 47 : this == NEW ? 35 : 4);
        int sigOffset = tail + (this == OLD ? 48 : this == NEW ? 36 : 0);
        String end = new String(b.read(sigOffset, 4), StandardCharsets.US_ASCII);
        if (!end.equals(this == BIG ? "oven" : signature) || sequence != b.get(seqOffset))
            throw new AdfsDirectoryException("Broken directory: header and tail differ");
        if (this != OLD && b.get(actual - 1) != checksum(b))
            throw new AdfsDirectoryException("Directory check byte mismatch");

        /* Parse the entry table, retaining old filename attributes and extended names. */
        ArrayList<DirectoryEntry> entries = new ArrayList<>();
        int nameLength = this == BIG ? Math.toIntExact(b.readLe(8, 4)) : 0;
        int start = this == BIG ? wordAlign(28 + nameLength + 1) : 5;
        int count = this == BIG ? Math.toIntExact(b.readLe(16, 4)) : maxEntries;
        if (this == BIG && (count < 0 || count > (tail - start) / 28))
            throw new AdfsDirectoryException("Invalid big-directory entry count");
        int heap = start + count * 28;
        for (int i = 0; i < count; i++)
        {
            int o = start + i * (this == BIG ? 28 : 26);
            if (this != BIG && b.get(o) == 0)
                break;
            String name;
            int access = 0;
            boolean directory;
            if (this == OLD)
            {
                int[] flags = {Access.R,
                               Access.W,
                               Access.L,
                               0,
                               Access.E,
                               Access.PR,
                               Access.PW,
                               Access.PE,
                               Access.PL};
                for (int j = 0; j < flags.length; j++)
                    if ((b.get(o + j) & 128) != 0)
                        access |= flags[j];
                directory = (b.get(o + 3) & 128) != 0;
                name = text(b, o, 10, true);
            }
            else
            {
                int flags = (int)b.readLe(o + (this == BIG ? 16 : 25), this == BIG ? 4 : 1);
                access = (flags & 3) | ((flags & 4) << 1) | (flags & 48);
                directory = (flags & 8) != 0;
                int n = this == BIG ? Math.toIntExact(b.readLe(o + 20, 4)) : 10;
                int ptr =
                    this == BIG ? Math.addExact(heap, Math.toIntExact(b.readLe(o + 24, 4))) : o;
                if (this == BIG && (n > 255 || ptr < heap || ptr + n > tail))
                    throw new AdfsDirectoryException("Invalid name heap pointer");
                name = text(b, ptr, n, false);
            }
            int v = o + (this == BIG ? 0 : 10);
            entries.add(new DirectoryEntry(name,
                                           b.readLe(v, 4),
                                           b.readLe(v + 4, 4),
                                           Math.toIntExact(b.readLe(v + 8, 4)),
                                           (int)b.readLe(v + 12, this == BIG ? 4 : 3),
                                           this == OLD ? b.get(o + 25) : 0,
                                           access,
                                           directory));
        }
        String name = this == BIG ? text(b, 28, nameLength, false)
                                  : text(b, tail + (this == OLD ? 1 : 25), 10, this == OLD);
        String title =
            this == BIG
                ? ""
                : text(b, tail + (this == OLD ? 14 : 6), 19, this == OLD).replaceFirst(" +$", "");
        int parent =
            (int)b.readLe(this == BIG ? 24 : tail + (this == OLD ? 11 : 3), this == BIG ? 4 : 3);
        return new Directory(
            name, title, parent, address, entries, sequence, signature, this == BIG ? actual : 0);
    }

    public byte[] serialize(Directory dir) throws AdfsDirectoryException
    {
        int actual = this == BIG ? Math.max(size, dir.bigSize()) : size;
        if (dir.entries().size() > maxEntries || requiredSize(dir, 1024) > actual)
            throw new AdfsDirectoryFullException("Directory full");
        if (this != BIG)
            validateTitle(dir.title());
        byte[] result = new byte[actual];
        SectorsView b = SectorsView.wrap(result);
        int sequence = dir.sequence() & 255;
        int tail = actual - (this == OLD ? 53 : this == NEW ? 41 : 8);
        String signature = this == BIG                      ? "SBPr"
                           : this == OLD                    ? "Hugo"
                           : dir.signature().equals("Nick") ? "Nick"
                                                            : "Hugo";
        b.put(0, sequence);
        b.write(this == BIG ? 4 : 1, signature.getBytes(StandardCharsets.US_ASCII));
        int start = this == BIG ? wordAlign(28 + dir.name().length() + 1) : 5;
        int heap = start + dir.entries().size() * 28;
        int ptr = 0;
        for (int i = 0; i < dir.entries().size(); i++)
        {
            DirectoryEntry e = dir.entries().get(i);
            int o = start + i * (this == BIG ? 28 : 26);
            if (this != BIG)
                field(b, o, 10, e.name(), this == OLD);
            if (this == OLD)
            {
                int[] flags = {Access.R,
                               Access.W,
                               Access.L,
                               0,
                               Access.E,
                               Access.PR,
                               Access.PW,
                               Access.PE,
                               Access.PL};
                for (int j = 0; j < flags.length; j++)
                    if ((e.access() & flags[j]) != 0 || (j == 3 && e.directory()))
                        b.put(o + j, b.get(o + j) | 128);
                b.put(o + 25, e.sequence() & 255);
            }
            else
            {
                int flags = (e.access() & 3) | ((e.access() & Access.L) >> 1) | (e.access() & 48) |
                            (e.directory() ? 8 : 0);
                b.writeLe(o + (this == BIG ? 16 : 25), flags, this == BIG ? 4 : 1);
            }
            int v = o + (this == BIG ? 0 : 10);
            b.writeLe(v, e.loadAddress(), 4);
            b.writeLe(v + 4, e.executionAddress(), 4);
            b.writeLe(v + 8, e.length(), 4);
            b.writeLe(v + 12, e.discAddress(), this == BIG ? 4 : 3);
            if (this == BIG)
            {
                field(b, heap + ptr, e.name().length() + 1, e.name(), false);
                b.writeLe(o + 20, e.name().length(), 4);
                b.writeLe(o + 24, ptr, 4);
                ptr += e.name().length() + 1;
            }
        }
        if (this == BIG)
        {
            b.writeLe(8, dir.name().length(), 4);
            b.writeLe(12, actual, 4);
            b.writeLe(16, dir.entries().size(), 4);
            b.writeLe(20, wordAlign(ptr), 4);
            b.writeLe(24, dir.parentAddress(), 4);
            field(b, 28, dir.name().length() + 1, dir.name(), false);
        }
        else
        {
            field(b, tail + (this == OLD ? 1 : 25), 10, dir.name(), this == OLD);
            field(b, tail + (this == OLD ? 14 : 6), 19, dir.title(), this == OLD);
            b.writeLe(tail + (this == OLD ? 11 : 3), dir.parentAddress(), 3);
        }
        b.put(tail + (this == OLD ? 47 : this == NEW ? 35 : 4), sequence);
        b.write(tail + (this == OLD   ? 48
                        : this == NEW ? 36
                                      : 0),
                (this == BIG ? "oven" : signature).getBytes(StandardCharsets.US_ASCII));
        if (this != OLD)
            b.put(actual - 1, checksum(b));
        return result;
    }

    public void serialize(Directory dir, SectorsView target) throws AdfsDirectoryException
    {
        byte[] bytes = serialize(dir);
        target.write(0, bytes);
    }

    /* FileCore rotates before XORing each complete word, then any final bytes. */
    public int checksum(SectorsView b)
    {
        int actual = this == BIG ? (int)b.readLe(12, 4) : size;
        int tail = actual - (this == OLD ? 53 : this == NEW ? 41 : 8);
        int end;
        if (this == BIG)
            end = wordAlign(28 + (int)b.readLe(8, 4) + 1) + (int)b.readLe(16, 4) * 28 +
                  (int)b.readLe(20, 4);
        else
        {
            end = 5;
            while (end < tail && b.get(end) != 0)
                end += 26;
        }
        if (end > tail)
            throw new IllegalArgumentException("Invalid directory bounds");
        int value = 0, offset = 0;
        while (offset + 4 <= end)
        {
            value = (int)b.readLe(offset, 4) ^ Integer.rotateRight(value, 13);
            offset += 4;
        }
        while (offset < end)
            value = b.get(offset++) ^ Integer.rotateRight(value, 13);
        offset = tail + (this == BIG ? 0 : 1);
        while (offset + 4 <= actual - 4)
        {
            value = (int)b.readLe(offset, 4) ^ Integer.rotateRight(value, 13);
            offset += 4;
        }
        if (this == BIG)
            while (offset < actual - 1)
                value = b.get(offset++) ^ Integer.rotateRight(value, 13);
        return (value ^ (value >>> 8) ^ (value >>> 16) ^ (value >>> 24)) & 255;
    }
}
