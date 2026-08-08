package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;

/**
 * Writes a D64 (Commodore 1541) sector image, ported from
 * lib/imagewriter/d64imagewriter.cc.
 */
public class D64ImageWriter extends ImageWriter
{
    public D64ImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    private static int sectorsPerTrack(int track)
    {
        if (track < 17)
            return 21;
        if (track < 24)
            return 19;
        if (track < 30)
            return 18;
        return 17;
    }

    @Override
    public void writeImage(Image image)
    {
        System.out.println("D64: writing triangular image");

        Bytes output = new Bytes();
        ByteWriter bw = output.writer();

        int offset = 0;
        for (int track = 0; track < 40; track++)
        {
            int sectorCount = sectorsPerTrack(track);
            for (int sectorId = 0; sectorId < sectorCount; sectorId++)
            {
                Sector sector = image.get(track, 0, sectorId);
                if (sector != null)
                {
                    bw.seek(offset);
                    bw.write(sector.data);
                }

                offset += 256;
            }
        }

        output.writeToFile(config.getFilename());
    }
}
