package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Reads a D64 (Commodore 1541) sector image, ported from
 * lib/imagereader/d64imagereader.cc.
 */
public class D64ImageReader extends ImageReader
{
    public D64ImageReader(ImageReaderProto config)
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
    public Image readImage()
    {
        Bytes data;
        try
        {
            data = new Bytes(Files.readAllBytes(Path.of(config.getFilename())));
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open input file");
        }
        int inputFileSize = data.size();

        int numCylinders = 39;
        int numHeads = 1;

        Logger.logf("D64: reading image with " + numCylinders + " tracks, " + numHeads + " heads");

        int offset = 0;

        Image image = new Image();
        for (int track = 0; track < 40; track++)
        {
            int numSectors = sectorsPerTrack(track);
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
                    Sector sector = image.put(track, head, sectorId);
                    if (offset < inputFileSize)
                    { /* still data available sector OK */
                        sector.status = Sector.Status.OK;
                        sector.data = data.slice(offset, 256);
                        offset += 256;
                    } else
                    { /* no more data in input file. Write sectors with status:
                     * DATA_MISSING */
                        sector.status = Sector.Status.DATA_MISSING;
                    }
                }
            }
        }

        image.calculateSize();
        return image;
    }
}
