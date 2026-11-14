package org.rob_smallshire.oaknut.adfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.SectorsView;
import org.rob_smallshire.oaknut.discimage.UnifiedDisc;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * FileCore allocation cells and FreeLink chains. Fragment order starts at the ID's home zone. */
public final class NewMap
{
    private final SectorsView map;
    private final UnifiedDisc disc;
    private final Map<Integer, List<Fragment>> fragments = new LinkedHashMap<>();
    private DiscRecord record;
    private int freeBytes;

    public NewMap(SectorsView map, DiscRecord record, UnifiedDisc disc) throws AdfsMapException
    {
        this.map = map;
        this.record = record;
        this.disc = disc;
        if (!record.looksValid())
            throw new AdfsMapException("Invalid disc record");
        rebuild();
    }

    public static int readBits(SectorsView data, int offset, int count)
    {
        int value = 0;
        for (int i = 0; i < count; i++)
            value |= ((data.get((offset + i) / 8) >>> ((offset + i) & 7)) & 1) << i;
        return value;
    }

    public static void writeBits(SectorsView data, int offset, int count, int value)
    {
        for (int i = 0; i < count; i++)
        {
            int byteOffset = (offset + i) / 8, bit = 1 << ((offset + i) & 7);
            data.put(
                    byteOffset,
                    (data.get(byteOffset) & ~bit) | (((value >>> i) & 1) != 0 ? bit : 0));
        }
    }

    public static int zoneChecksum(SectorsView b, int zone, int log2SectorSize)
    {
        int start = zone << log2SectorSize;
        long a = 0, c = 0, d = 0, e = 0;
        int pos = ((zone + 1) << log2SectorSize) - 4;
        for (; pos > start; pos -= 4)
        {
            a += b.get(pos) + (e >>> 8);
            e &= 255;
            c += b.get(pos + 1) + (a >>> 8);
            a &= 255;
            d += b.get(pos + 2) + (c >>> 8);
            c &= 255;
            e += b.get(pos + 3) + (d >>> 8);
            d &= 255;
        }
        a += e >>> 8;
        c += b.get(pos + 1) + (a >>> 8);
        d += b.get(pos + 2) + (c >>> 8);
        e += b.get(pos + 3) + (d >>> 8);
        return (int) (a ^ c ^ d ^ e) & 255;
    }

    /* Format blank single- and multi-zone floppy maps, including the boot record. */
    public static int format(SectorsView image, DiscRecord dr)
    {
        int sec = dr.sectorSize(), bpmb = dr.bytesPerMapBit(), n = dr.zones();
        int bootmap = dr.bootmap();
        SectorsView map = image.slice(bootmap, n * sec * 2);
        dr.serialize(map, 4);
        int rootSize = 2048;
        if (n == 1)
        {
            int systemBits = (sec * 2 + (dr.big() ? 0 : rootSize)) / bpmb;
            writeBits(map, 512, dr.idLength(), 2);
            writeBits(map, 512 + systemBits - 1, 1, 1);
            int free = 512 + systemBits;
            if (dr.big())
            {
                writeBits(map, free, dr.idLength(), dr.root() >>> 8);
                writeBits(map, free + rootSize / bpmb - 1, 1, 1);
                free += rootSize / bpmb;
            }
            writeBits(map, 512 + dr.discSize() / bpmb - 1, 1, 1);
            map.writeLe(1, 0x8000 | (free - 8), 2);
        } else
        {
            image.put(0xc03, 0x20);
            dr.serialize(image, 0xdc0);
            image.write(0xdc0 + 20, new byte[40]);
            image.put(0xdc7, 0);
            image.put(0xdff, bootChecksum(image.slice(0xc00, 512)));
            int middle = n / 2;
            for (int zone = 0; zone < n; zone++)
            {
                int start = zone * sec * 8 + (zone == 0 ? 512 : 32);
                int free = start;
                if (zone == 0 || zone == middle)
                {
                    int bits = (zone == 0 ? 4096 : n * sec * 2 + (dr.big() ? 0 : rootSize)) / bpmb;
                    writeBits(map, start, dr.idLength(), 2);
                    writeBits(map, start + bits - 1, 1, 1);
                    free += bits;
                }
                if (zone == middle && dr.big())
                {
                    writeBits(map, free, dr.idLength(), dr.root() >>> 8);
                    writeBits(map, free + rootSize / bpmb - 1, 1, 1);
                    free += rootSize / bpmb;
                }
                writeBits(map, zone * sec * 8 + 32 + sec * 8 - dr.zoneSpare() - 1, 1, 1);
                map.writeLe(zone * sec + 1, 0x8000 | (free - (zone * sec + 1) * 8), 2);
            }
            int eod = dr.discSize() / bpmb + dr.zoneSpare() * (n - 1) + 512;
            writeBits(map, eod - 1, 1, 1);
            writeBits(map, eod, dr.idLength(), 1);
        }
        map.put((n - 1) * sec + 3, 255);
        for (int zone = 0; zone < n; zone++)
            map.put(zone * sec, zoneChecksum(map, zone, dr.log2SectorSize()));
        map.write(n * sec, map.read(0, n * sec));
        return bootmap + n * sec * 2;
    }

    public static int bootChecksum(SectorsView b)
    {
        int sum = 0;
        for (int i = 0; i < 511; i++)
        {
            sum += b.get(i);
            if (sum > 255)
                sum = (sum & 255) + 1;
        }
        return sum & 255;
    }

    public DiscRecord discRecord()
    {
        return record;
    }

    public int freeSpace()
    {
        return freeBytes;
    }

    public int minFragmentBytes()
    {
        return (record.idLength() + 1) * record.bytesPerMapBit();
    }

    private int idPerZone()
    {
        return (record.sectorSize() * 8 - record.zoneSpare()) / (record.idLength() + 1);
    }

    private int start(int zone)
    {
        return zone * record.sectorSize() * 8 + (zone == 0 ? 512 : 32);
    }

    private int end(int zone)
    {
        return record.zones() == 1 ?
                512 + record.discSize() / record.bytesPerMapBit() :
                zone * record.sectorSize() * 8 + 32 + record.sectorSize() * 8 - record.zoneSpare();
    }

    private Set<Integer> freePositions(int zone) throws AdfsMapException
    {
        HashSet<Integer> positions = new HashSet<>();
        int header = (zone * record.sectorSize() + 1) * 8;
        int link = readBits(map, header, 16) & 0x7fff;
        int pos = header;
        while (link != 0)
        {
            pos += link;
            if (pos < start(zone) || pos + record.idLength() >= end(zone) || !positions.add(pos))
                throw new AdfsMapException("Invalid FreeLink chain in zone " + zone);
            link = readBits(map, pos, record.idLength());
        }
        return positions;
    }

    private List<Cell> cells(int zone) throws AdfsMapException
    {
        Set<Integer> free = freePositions(zone);
        ArrayList<Cell> result = new ArrayList<>();
        int limit = end(zone);
        for (int pos = start(zone); pos < limit; )
        {
            int stop = pos + record.idLength();
            while (stop < limit && readBits(map, stop, 1) == 0)
                stop++;
            if (stop >= limit)
                throw new AdfsMapException("Unterminated allocation cell in zone " + zone);
            result.add(new Cell(
                    free.contains(pos) ? null : readBits(map, pos, record.idLength()),
                    stop - pos + 1));
            pos = stop + 1;
        }
        return result;
    }

    private void rebuild() throws AdfsMapException
    {
        fragments.clear();
        freeBytes = 0;
        for (int zone = 0; zone < record.zones(); zone++)
        {
            int pos = start(zone);
            for (Cell cell : cells(zone))
            {
                int capacity = cell.bits() * record.bytesPerMapBit();
                if (cell.id() == null)
                    freeBytes += capacity;
                else if (cell.id() > 0)
                {
                    int address = Math.floorMod(
                            (pos - 512 - record.zoneSpare() * zone) * record.bytesPerMapBit(),
                            record.discSize());
                    fragments.computeIfAbsent(cell.id(), k -> new ArrayList<>())
                            .add(new Fragment(address, capacity, zone));
                }
                pos += cell.bits();
            }
        }
    }

    public ImmutableList<Fragment> fragments(int id)
    {
        ArrayList<Fragment> result = new ArrayList<>(fragments.getOrDefault(id, List.of()));
        int home = id / idPerZone();
        result.sort(Comparator.comparingInt(f -> Math.floorMod(f.zone() - home, record.zones())));
        return ImmutableList.copyOf(result);
    }

    public int fragmentCapacity(int id)
    {
        return fragments(id).stream().mapToInt(Fragment::capacity).sum();
    }

    public int fragmentCount(int id)
    {
        return fragments(id).size();
    }

    private int within(int indirect)
    {
        return Math.max(0, (indirect & 255) - 1) * record.sectorSize();
    }

    private boolean systemRoot(int indirect)
    {
        return indirect == record.root() && (indirect >>> 8) == 2;
    }

    public int rootPhysical()
    {
        return record.bootmap() + record.zones() * record.sectorSize() * 2;
    }

    public int objectStart(int indirect) throws AdfsMapException
    {
        if (systemRoot(indirect))
            return rootPhysical();
        List<Fragment> spans = fragments(indirect >>> 8);
        if (spans.isEmpty())
            throw new AdfsMapException("No fragment for indirect address " + indirect);
        return spans.getFirst().address() + within(indirect);
    }

    private byte[] readPhysical(int offset, int length)
    {
        byte[] result = new byte[length];
        int cursor = 0;
        while (cursor < length)
        {
            int at = Math.floorMod(offset + cursor, record.discSize());
            int take = Math.min(length - cursor, record.discSize() - at);
            byte[] part = disc.bytes(at, take).toByteArray();
            System.arraycopy(part, 0, result, cursor, take);
            cursor += take;
        }
        return result;
    }

    public byte[] readObject(int indirect, int length) throws AdfsMapException
    {
        if (systemRoot(indirect))
            return readPhysical(rootPhysical(), length);
        byte[] output = new byte[length];
        int cursor = 0, skip = within(indirect);
        for (Fragment f : fragments(indirect >>> 8))
        {
            int take = Math.min(length - cursor, Math.max(0, f.capacity() - skip));
            if (take > 0)
            {
                System.arraycopy(readPhysical(f.address() + skip, take), 0, output, cursor, take);
                cursor += take;
            }
            skip = Math.max(0, skip - f.capacity());
            if (cursor == length)
                return output;
        }
        if (cursor != length)
            throw new AdfsMapException("Object is shorter than its catalogued length");
        return output;
    }

    public void writeObjectData(int indirect, byte[] bytes) throws AdfsMapException
    {
        if (systemRoot(indirect))
        {
            disc.bytes(rootPhysical(), bytes.length).write(0, bytes);
            return;
        }
        int capacity = fragmentCapacity(indirect >>> 8) - within(indirect);
        if (capacity < bytes.length)
            throw new AdfsMapException("Object exceeds fragment capacity");
        int cursor = 0, skip = within(indirect);
        for (Fragment f : fragments(indirect >>> 8))
        {
            int take = Math.min(bytes.length - cursor, Math.max(0, f.capacity() - skip));
            int consumed = 0;
            while (consumed < take)
            {
                int at = Math.floorMod(f.address() + skip + consumed, record.discSize());
                int part = Math.min(take - consumed, record.discSize() - at);
                disc.bytes(at, part).write(
                        0,
                        java.util.Arrays.copyOfRange(
                                bytes,
                                cursor + consumed,
                                cursor + consumed + part));
                consumed += part;
            }
            cursor += take;
            skip = Math.max(0, skip - f.capacity());
            if (cursor == bytes.length)
                return;
        }
    }

    private void rewrite(int zone, List<Cell> cells) throws AdfsMapException
    {
        int pos = start(zone);
        for (int i = pos / 8; i < (end(zone) + 7) / 8; i++)
            map.put(i, 0);
        int previous = (zone * record.sectorSize() + 1) * 8;
        boolean first = true;
        map.writeLe(zone * record.sectorSize() + 1, 0x8000, 2);
        for (Cell cell : cells)
        {
            if (cell.id() != null)
                writeBits(map, pos, record.idLength(), cell.id());
            else
            {
                if (first)
                    map.writeLe(zone * record.sectorSize() + 1, 0x8000 | (pos - previous), 2);
                else
                    writeBits(map, previous, record.idLength(), pos - previous);
                previous = pos;
                first = false;
            }
            writeBits(map, pos + cell.bits() - 1, 1, 1);
            pos += cell.bits();
        }
        refresh(zone);
        rebuild();
    }

    public void refresh(int zone)
    {
        int sec = record.sectorSize();
        map.put(zone * sec, zoneChecksum(map, zone, record.log2SectorSize()));
        map.write((record.zones() + zone) * sec, map.read(zone * sec, sec));
    }

    private int needBits(int length)
    {
        int bpmb = record.bytesPerMapBit();
        int alignment = Math.max(1, record.sectorSize() / bpmb);
        int need = Math.max((length + bpmb - 1) / bpmb, record.idLength() + 1);
        return (need + alignment - 1) / alignment * alignment;
    }

    private List<Cell> fill(List<Cell> cells, int id, int needed)
    {
        ArrayList<Cell> result = new ArrayList<>();
        for (Cell c : cells)
        {
            if (c.id() != null || needed <= 0)
            {
                result.add(c);
                continue;
            }
            int take = Math.min(needed, c.bits());
            if (c.bits() - take < record.idLength() + 1)
                take = c.bits();
            result.add(new Cell(id, take));
            if (take < c.bits())
                result.add(new Cell(null, c.bits() - take));
            needed -= take;
        }
        return result;
    }

    public int allocateObject(int length) throws AdfsMapException
    {
        if (length < 0)
            throw new IllegalArgumentException("Negative object size");
        int need = needBits(length);
        for (int home = 0; home < record.zones(); home++)
        {
            int id = Math.max(3, home * idPerZone());
            int limit = record.zones() == 1 ? 1 << record.idLength() : (home + 1) * idPerZone();
            while (id < limit && fragments.containsKey(id))
                id++;
            if (id == limit)
                continue;
            LinkedHashMap<Integer, List<Cell>> plan = new LinkedHashMap<>();
            int remaining = need;
            for (int k = 0; k < record.zones() && remaining > 0; k++)
            {
                int zone = (home + k) % record.zones();
                List<Cell> filled = fill(cells(zone), id, remaining);
                int allocatedId = id;
                int taken = filled
                        .stream()
                        .filter(c -> c.id() != null && c.id() == allocatedId)
                        .mapToInt(Cell::bits)
                        .sum();
                if (taken > 0)
                {
                    plan.put(zone, filled);
                    remaining -= taken;
                }
            }
            if (remaining > 0)
                continue;
            for (Map.Entry<Integer, List<Cell>> e : plan.entrySet())
                rewrite(e.getKey(), e.getValue());
            return id << 8;
        }
        throw new AdfsDiscFullException("Not enough free space for " + length + " bytes");
    }

    public void freeObject(int indirect) throws AdfsMapException
    {
        int id = indirect >>> 8;
        if (!fragments.containsKey(id))
            throw new AdfsMapException("No fragment " + id);
        for (int zone = 0; zone < record.zones(); zone++)
        {
            ArrayList<Cell> result = new ArrayList<>();
            boolean changed = false;
            for (Cell c : cells(zone))
            {
                boolean match = c.id() != null && c.id() == id;
                changed |= match;
                Cell next = match ? new Cell(null, c.bits()) : c;
                if (next.id() == null && !result.isEmpty() && result.getLast().id() == null)
                {
                    Cell prev = result.removeLast();
                    next = new Cell(null, prev.bits() + next.bits());
                }
                result.add(next);
            }
            if (changed)
                rewrite(zone, result);
        }
    }

    public void growFragment(int indirect, int length) throws AdfsMapException
    {
        int id = indirect >>> 8;
        if (fragmentCount(id) != 1)
            throw new AdfsMapException("Not a single fragment");
        for (int zone = 0; zone < record.zones(); zone++)
        {
            ArrayList<Cell> cells = new ArrayList<>(cells(zone));
            for (int i = 0; i < cells.size(); i++)
            {
                Cell c = cells.get(i);
                if (c.id() == null || c.id() != id)
                    continue;
                int grow = needBits(length) - c.bits();
                if (grow <= 0)
                    return;
                if (i + 1 == cells.size() || cells.get(i + 1).id() != null ||
                        cells.get(i + 1).bits() < grow)
                    throw new AdfsDiscFullException("No adjacent space to grow directory");
                Cell free = cells.remove(i + 1);
                int left = free.bits() - grow;
                if (left < record.idLength() + 1)
                    grow += left;
                else
                    cells.add(i + 1, new Cell(null, left));
                cells.set(i, new Cell(id, c.bits() + grow));
                rewrite(zone, cells);
                return;
            }
        }
    }

    public void setRoot(int indirect, int size)
    {
        record = record.withRoot(indirect, record.rootSize());
        record.serialize(map, 4);
        refresh(0);
    }

    public void setDiscName(String name)
    {
        record = record.withName(name);
        record.serialize(map, 4);
        refresh(0);
    }

    public void setBootOption(int option)
    {
        record = record.withBootOption(option);
        record.serialize(map, 4);
        refresh(0);
    }

    public ImmutableList<String> validate()
    {
        ImmutableList.Builder<String> errors = ImmutableList.builder();
        for (int zone = 0; zone < record.zones(); zone++)
            if (map.get(zone * record.sectorSize()) !=
                    zoneChecksum(map, zone, record.log2SectorSize()))
                errors.add("Zone " + zone + " check byte mismatch");
        return errors.build();
    }

    public record Fragment(int address, int capacity, int zone)
    {
    }

    private record Cell(Integer id, int bits)
    {
    }
}
