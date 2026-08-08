package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/* Image reader for Northstar floppy disk images */
public class NsiImageReader extends ImageReader
{
    public NsiImageReader(ImageReaderProto config)
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
        int fsize = data.size();

        Logger.logf("NSI: Autodetecting geometry based on file size: " + fsize);

        int numCylinders = 35;
        int numSectors = 10;
        int numHeads = 2;
        int sectorSize = 512;

        switch (fsize)
        {
            case 358400:
                numHeads = 2;
                sectorSize = 512;
                break;

            case 179200:
                numHeads = 1;
                sectorSize = 512;
                break;

            case 89600:
                numHeads = 1;
                sectorSize = 256;
                break;

            default:
                throw new FluxEngineException("NSI: unknown file size");
        }

        int trackSize = numSectors * sectorSize;

        Logger.logf("reading " + numCylinders + " tracks, " + numHeads + " heads, " + numSectors +
                " sectors, " + sectorSize + " bytes per sector, " +
                numCylinders * numHeads * trackSize / 1024 + " kB total");

        Image image = new Image();
        ByteReader br = new ByteReader(data);
        int sectorFileOffset;

        for (int head = 0; head < numHeads; head++)
        {
            for (int track = 0; track < numCylinders; track++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
                    if (head == 0)
                    { /* Head 0 is from track 0-34 */
                        sectorFileOffset = track * trackSize + sectorId * sectorSize;
                    } else
                    { /* Head 1 is from track 70-35 */
                        sectorFileOffset = (trackSize * numCylinders) + /* Skip over side 0 */
                                ((numCylinders - track - 1) * trackSize) + (sectorId *
                                sectorSize); /* Sector offset from beginning of track. */
                    }

                    br.seek(sectorFileOffset);
                    Bytes sectorData = br.read(sectorSize);

                    Sector sector = image.put(track, head, sectorId);
                    sector.status = Sector.Status.OK;
                    sector.data = sectorData;
                }
            }
        }

        Geometry geometry = new Geometry();
        geometry.numCylinders = numCylinders;
        geometry.numHeads = numHeads;
        geometry.numSectors = numSectors;
        geometry.sectorSize = sectorSize;
        image.setGeometry(geometry);
        return image;
    }
}
