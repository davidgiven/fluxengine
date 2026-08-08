package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Writes a D88 sector image, ported from lib/imagewriter/d88imagewriter.cc.
 */
public class D88ImageWriter extends ImageWriter
{
    public D88ImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    private static int countlZero(int value)
    {
        int count = 0;
        while ((value & 0x80000000) == 0)
        {
            value <<= 1;
            count++;
        }
        return count;
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();

        int tracks = geometry.numCylinders;
        int sides = geometry.numHeads;

        Bytes header = new Bytes();
        ByteWriter headerWriter = header.writer();
        for (int i = 0; i < 26; i++)
        {
            headerWriter.write8(0x0); /* image name + reserved bytes */
        }
        headerWriter.write8(0x00); /* not write protected */
        if (geometry.numCylinders > 42)
        {
            headerWriter.write8(0x20); /* 2HD */
        } else
        {
            headerWriter.write8(0x00); /* 2D */
        }
        headerWriter.writeLe32(0); /* disk size (overridden at the end) */
        for (int i = 0; i < 164; i++)
        {
            headerWriter.writeLe32(0); /* track pointer (overridden in loop) */
        }

        Bytes output = header;
        ByteWriter bw = output.writer();

        int trackOffset = 688;

        for (int track = 0; track < geometry.numCylinders * geometry.numHeads; track++)
        {
            headerWriter.seek(0x20 + 4 * track);
            headerWriter.writeLe32(trackOffset);
            int side = track & 1;
            List<Sector> sectors = new ArrayList<>();
            for (int sectorId = geometry.firstSector; sectorId <= geometry.numSectors;
                    sectorId++)
            {
                Sector sector = image.get(track >> 1, side, sectorId);
                if (sector != null)
                    sectors.add(sector);
            }
            sectors.sort(Comparator.comparingInt(s -> s.position));
            for (Sector sector : sectors)
            {
                Bytes sectorBytes = new Bytes();
                ByteWriter sectorWriter = sectorBytes.writer();
                sectorWriter.write8(sector.location.logicalCylinder());
                sectorWriter.write8(sector.location.logicalHead());
                sectorWriter.write8(sector.location.logicalSector());
                sectorWriter.write8(24 - countlZero(sector.data.size()));
                sectorWriter.writeLe16(sectors.size());
                sectorWriter.write8(0x00); /* always write mfm */
                sectorWriter.write8(0x00); /* always write not deleted data */
                if (sector.status == Sector.Status.BAD_CHECKSUM)
                {
                    sectorWriter.write8(0xB0);
                } else
                {
                    sectorWriter.write8(0x00);
                }
                sectorWriter.write8(0x00); /* reserved */
                sectorWriter.write8(0x00);
                sectorWriter.write8(0x00);
                sectorWriter.write8(0x00);
                sectorWriter.write8(0x00);
                sectorWriter.writeLe16(sector.data.size());
                output = output.concat(sectorBytes);
                output = output.concat(sector.data);
                trackOffset += sectorBytes.size();
                trackOffset += sector.data.size();
            }
        }

        headerWriter.seek(0x1c);
        headerWriter.writeLe32(output.size());

        output.writeToFile(config.getFilename());

        System.out.printf("D88: wrote %d tracks, %d sides, %d kB total%n",
                tracks, sides, output.size() / 1024);
    }
}
