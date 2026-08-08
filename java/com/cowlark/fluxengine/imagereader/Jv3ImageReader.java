package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/* JV3 files are kinda weird. There's a fixed layout for up to 2901 sectors,
 * which may appear in any order, followed by the same again for more sectors.
 * To find the second data block you need to know the size of the first data
 * block, which requires parsing it.
 *
 * https://www.tim-mann.org/trs80/dskconfig.html
 */
public class Jv3ImageReader extends ImageReader
{
    private static final int JV3_DENSITY = 0x80; /* 1=dden, 0=sden */
    private static final int JV3_DAM = 0x60;     /* data address mark code */
    private static final int JV3_SIDE = 0x10;    /* 0=side 0, 1=side 1 */
    private static final int JV3_ERROR = 0x08;   /* 0=ok, 1=CRC error */
    private static final int JV3_NONIBM = 0x04;  /* 0=normal, 1=short */
    private static final int JV3_SIZE =
            0x03; /* in used sectors: 0=256,1=128,2=1024,3=512
                     in free sectors: 0=512,1=1024,2=128,3=256 */

    private static final int JV3_FREE = 0xFF;  /* in track and sector fields of free sectors */
    private static final int JV3_FREEF = 0xFC; /* in flags field, or'd with size code */

    private static int getSectorSize(int flags)
    {
        if ((flags & JV3_FREEF) == JV3_FREEF)
        {
            switch (flags & JV3_SIZE)
            {
                case 0:
                    return 512;
                case 1:
                    return 1024;
                case 2:
                    return 128;
                case 3:
                    return 256;
            }
        }
        else
        {
            switch (flags & JV3_SIZE)
            {
                case 0:
                    return 256;
                case 1:
                    return 128;
                case 2:
                    return 1024;
                case 3:
                    return 512;
            }
        }
        throw new FluxEngineException("not reachable");
    }

    public Jv3ImageReader(ImageReaderProto config)
    {
        super(config);
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
        int headerPtr = 0;
        Image image = new Image();
        for (; ; )
        {
            int dataPtr = headerPtr + 2901 * 3 + 1;
            if (dataPtr >= inputFileSize)
                break;

            for (int i = 0; i < 2901; i++)
            {
                ByteReader headerReader = new ByteReader(data.slice(headerPtr, 3));
                int track = headerReader.seek(0).read8();
                int sectorId = headerReader.seek(1).read8();
                int flags = headerReader.seek(2).read8();
                int sectorSize = getSectorSize(flags);
                if ((flags & JV3_FREEF) != JV3_FREEF)
                {
                    Bytes sectorData = data.slice(dataPtr, sectorSize);

                    int head = (flags & JV3_SIDE) != 0 ? 1 : 0;
                    Sector sector = image.put(track, head, sectorId);
                    sector.status = Sector.Status.OK;
                    sector.data = sectorData;
                }

                headerPtr += 3;
                dataPtr += sectorSize;
            }

            /* dataPtr is now pointing at the beginning of the next chunk. */

            headerPtr = dataPtr;
        }

        image.calculateSize();
        return image;
    }
}
