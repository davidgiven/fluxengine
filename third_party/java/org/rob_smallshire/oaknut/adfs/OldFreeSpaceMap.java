package org.rob_smallshire.oaknut.adfs;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.SectorsView;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * First-fit allocation in the two 256-byte old-map sectors. */
public final class OldFreeSpaceMap
{
    private final SectorsView data;

    public OldFreeSpaceMap(SectorsView data) throws AdfsMapException
    {
        this(data, true);
    }

    public OldFreeSpaceMap(SectorsView data, boolean verify) throws AdfsMapException
    {
        if (data.size() < 512)
            throw new AdfsMapException("Free space map needs 512 bytes");
        this.data = data.slice(0, 512);
        if (verify && !validate().isEmpty())
            throw new AdfsMapException(String.join("; ", validate()));
    }

    public static int checksum(SectorsView data, int start)
    {
        int total = 0, carry = 0;
        for (int i = 254; i >= 0; i--)
        {
            total += data.get(start + i) + carry;
            carry = total > 255 ? 1 : 0;
            total &= 255;
        }
        return total;
    }

    public ImmutableList<String> validate()
    {
        ImmutableList.Builder<String> errors = ImmutableList.builder();
        for (int sector = 0; sector < 2; sector++)
            if (data.get(sector * 256 + 255) != checksum(data, sector * 256))
                errors.add("Free-space map sector " + sector + " checksum mismatch");
        if (data.get(510) % 3 != 0 || data.get(510) > 246)
            errors.add("Invalid FreeEnd pointer " + data.get(510));
        return errors.build();
    }

    public void refresh()
    {
        data.put(255, checksum(data, 0));
        data.put(511, checksum(data, 256));
    }

    public int totalSectors()
    {
        return (int) data.readLe(252, 3);
    }

    public int totalSize()
    {
        return totalSectors() * 256;
    }

    public int discId()
    {
        return (int) data.readLe(507, 2);
    }

    public int bootOption()
    {
        return data.get(509);
    }

    public void setBootOption(int option)
    {
        data.put(509, option & 255);
        refresh();
    }

    public int freeSpace()
    {
        return entries().stream().mapToInt(Extent::length).sum() * 256;
    }

    public ImmutableList<Extent> entries()
    {
        ImmutableList.Builder<Extent> result = ImmutableList.builder();
        for (int i = 0; i < Math.min(82, data.get(510) / 3); i++)
            result.add(new Extent((int) data.readLe(i * 3, 3), (int) data.readLe(256 + i * 3, 3)));
        return result.build();
    }

    public void replaceEntries(List<Extent> entries) throws AdfsMapException
    {
        if (entries.size() > 82)
            throw new AdfsMapException("Free space map full");
        data.write(0, new byte[246]);
        data.write(256, new byte[246]);
        for (int i = 0; i < entries.size(); i++)
        {
            data.writeLe(i * 3, entries.get(i).start(), 3);
            data.writeLe(256 + i * 3, entries.get(i).length(), 3);
        }
        data.put(510, entries.size() * 3);
        refresh();
    }

    public int allocate(int sectors) throws AdfsMapException
    {
        return allocate(sectors, 1, totalSectors());
    }

    public int allocate(int sectors, int alignment, int limit) throws AdfsMapException
    {
        if (sectors <= 0 || alignment <= 0)
            throw new IllegalArgumentException("Invalid allocation size");
        ArrayList<Extent> entries = new ArrayList<>(entries());
        for (int i = 0; i < entries.size(); i++)
        {
            Extent e = entries.get(i);
            int start = (e.start() + alignment - 1) / alignment * alignment;
            int end = start + sectors;
            if (end > e.start() + e.length() || end > limit)
                continue;
            entries.remove(i);
            if (end < e.start() + e.length())
                entries.add(i, new Extent(end, e.start() + e.length() - end));
            if (start > e.start())
                entries.add(i, new Extent(e.start(), start - e.start()));
            replaceEntries(entries);
            return start;
        }
        throw new AdfsDiscFullException("No contiguous free region for " + sectors + " sectors");
    }

    public void free(int start, int sectors) throws AdfsMapException
    {
        if (sectors <= 0)
            throw new IllegalArgumentException("Invalid release size");
        ArrayList<Extent> entries = new ArrayList<>(entries());
        entries.add(new Extent(start, sectors));
        entries.sort(Comparator.comparingInt(Extent::start));
        ArrayList<Extent> merged = new ArrayList<>();
        for (Extent e : entries)
        {
            if (!merged.isEmpty())
            {
                Extent prev = merged.getLast();
                if (prev.start() + prev.length() == e.start())
                {
                    merged.set(
                            merged.size() - 1,
                            new Extent(prev.start(), prev.length() + e.length()));
                    continue;
                }
            }
            merged.add(e);
        }
        replaceEntries(merged);
    }

    public void shrinkTo(int sectors) throws AdfsMapException
    {
        if (sectors < 1 || sectors > totalSectors())
            throw new IllegalArgumentException("Invalid disc size");
        List<Extent> trimmed = entries()
                .stream()
                .filter(e -> e.start() < sectors)
                .map(e -> new Extent(e.start(), Math.min(e.length(), sectors - e.start())))
                .toList();
        replaceEntries(trimmed);
        data.writeLe(252, sectors, 3);
        refresh();
    }

    public long[] afsInfoPointers()
    {
        return new long[]{data.readLe(246, 4), data.readLe(502, 4)};
    }

    public void installAfsPointers(long first, long second)
    {
        if (first < 0 || first > 0xffffffffL || second < 0 || second > 0xffffffffL)
            throw new IllegalArgumentException("Invalid AFS pointer");
        data.writeLe(246, first, 4);
        data.writeLe(502, second, 4);
        refresh();
    }

    public String discName()
    {
        StringBuilder name = new StringBuilder();
        for (int i = 0; i < 5; i++)
            name.append((char) data.get(502 + i)).append((char) data.get(247 + i));
        return name.toString().replaceFirst("[\\x00 ]+$", "");
    }

    public void setDiscName(String name)
    {
        org.rob_smallshire.oaknut.file.AcornEncoding.checkTitle(name, 10, true);
        String padded = String.format("%-10s", name);
        for (int i = 0; i < 5; i++)
        {
            data.put(502 + i, padded.charAt(i * 2));
            data.put(247 + i, padded.charAt(i * 2 + 1));
        }
        refresh();
    }

    public record Extent(int start, int length)
    {
    }
}
