package org.rob_smallshire.oaknut.adfs;

import org.rob_smallshire.oaknut.discimage.SectorsView;
import org.rob_smallshire.oaknut.file.AcornEncoding;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * The floppy-relevant fields of the 60-byte FileCore disc record. */
public record DiscRecord(int log2SectorSize, int sectorsPerTrack, int heads, int density,
                         int idLength, int log2BytesPerMapBit, int skew, int bootOption,
                         int lowSector, int zones, int zoneSpare, int root, int discSize,
                         int discId, String discName, long discType, int formatVersion,
                         int rootSize)
{
    public static DiscRecord parse(SectorsView b, int o)
    {
        return new DiscRecord(
                b.get(o),
                b.get(o + 1),
                b.get(o + 2),
                b.get(o + 3),
                b.get(o + 4),
                b.get(o + 5),
                b.get(o + 6),
                b.get(o + 7),
                b.get(o + 8),
                b.get(o + 9),
                (int) b.readLe(o + 10, 2),
                (int) b.readLe(o + 12, 4),
                Math.toIntExact(b.readLe(o + 16, 4)),
                (int) b.readLe(o + 20, 2),
                AcornEncoding.decode(b.read(o + 22, 10), false).replaceFirst(" +$", ""),
                b.readLe(o + 32, 4),
                (int) b.readLe(o + 44, 4),
                (int) b.readLe(o + 48, 4));
    }

    public static DiscRecord forFormat(AdfsFormat f, String title, int id, int boot)
    {
        if (!f.newMap())
            throw new IllegalArgumentException("Not a new-map format");
        int spare = f.zones == 1 ? 1312 : 1600;
        int root = 0x200 | (f.zones * 2 + 1);
        if (f.big())
            root = ((f.zones == 1 ? 3 : (f.zones / 2) * ((8192 - spare) / 16)) << 8) | 1;
        return new DiscRecord(
                10,
                f.sectorsPerTrack,
                2,
                f.zones == 1 ? 2 : f.zones == 4 ? 4 : 8,
                15,
                f.zones == 1 ? 7 : 6,
                1,
                boot,
                0,
                f.zones,
                spare,
                root,
                f.size(),
                id,
                title,
                f.big() ? 0x20158318L : 0x20158c78L,
                f.big() ? 1 : 0,
                f.big() ? 2048 : 0);
    }

    public int sectorSize()
    {
        return 1 << log2SectorSize;
    }

    public int bytesPerMapBit()
    {
        return 1 << log2BytesPerMapBit;
    }

    public boolean big()
    {
        return formatVersion >= 1;
    }

    public boolean looksValid()
    {
        return log2SectorSize >= 8 && log2SectorSize <= 12 && zones > 0 &&
                idLength >= log2SectorSize + 3 && idLength < 22;
    }

    public int bootmap()
    {
        return zones <= 1 ?
                0 :
                ((zones / 2) * (8 * sectorSize() - zoneSpare) - (zones > 2 ? 480 : 0)) *
                        bytesPerMapBit();
    }

    public DiscRecord withRoot(int root, int size)
    {
        return new DiscRecord(
                log2SectorSize,
                sectorsPerTrack,
                heads,
                density,
                idLength,
                log2BytesPerMapBit,
                skew,
                bootOption,
                lowSector,
                zones,
                zoneSpare,
                root,
                discSize,
                discId,
                discName,
                discType,
                formatVersion,
                size);
    }

    public DiscRecord withName(String name)
    {
        return new DiscRecord(
                log2SectorSize,
                sectorsPerTrack,
                heads,
                density,
                idLength,
                log2BytesPerMapBit,
                skew,
                bootOption,
                lowSector,
                zones,
                zoneSpare,
                root,
                discSize,
                discId,
                name,
                discType,
                formatVersion,
                rootSize);
    }

    public DiscRecord withBootOption(int option)
    {
        return new DiscRecord(
                log2SectorSize,
                sectorsPerTrack,
                heads,
                density,
                idLength,
                log2BytesPerMapBit,
                skew,
                option,
                lowSector,
                zones,
                zoneSpare,
                root,
                discSize,
                discId,
                discName,
                discType,
                formatVersion,
                rootSize);
    }

    public void serialize(SectorsView b, int o)
    {
        int[] bytes = {log2SectorSize,
                sectorsPerTrack,
                heads,
                density,
                idLength,
                log2BytesPerMapBit,
                skew,
                bootOption,
                lowSector,
                zones};
        for (int i = 0; i < bytes.length; i++)
            b.put(o + i, bytes[i]);
        b.writeLe(o + 10, zoneSpare, 2);
        b.writeLe(o + 12, root, 4);
        b.writeLe(o + 16, discSize, 4);
        b.writeLe(o + 20, discId, 2);
        byte[] name = AcornEncoding.encode(discName, false);
        b.write(o + 22, java.util.Arrays.copyOf(name, 10));
        b.writeLe(o + 32, discType, 4);
        b.writeLe(o + 44, formatVersion, 4);
        b.writeLe(o + 48, rootSize, 4);
    }
}
