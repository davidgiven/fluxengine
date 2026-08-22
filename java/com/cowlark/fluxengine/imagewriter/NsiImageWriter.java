package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;

/**
 * Writes an NSI (North Star) sector image, ported from
 * lib/imagewriter/nsiimagewriter.cc.
 */
public class NsiImageWriter extends ImageWriter
{
    public NsiImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();
        boolean mixedDensity = false;

        int trackSize = geometry.numSectors * geometry.sectorSize;

        if (geometry.numCylinders * trackSize == 0)
        {
            Logger.logf("No sectors in output; skipping .nsi image file generation.");
            return;
        }

        Logger.logf(
                "Writing %d tracks, %d sides, %d sectors, %s (%d bytes/sector), " + "%d kB total%n",
                geometry.numCylinders,
                geometry.numHeads,
                geometry.numSectors,
                geometry.sectorSize == 256 ? "SD" : "DD",
                geometry.sectorSize,
                geometry.numCylinders * geometry.numHeads * geometry.numSectors *
                        geometry.sectorSize / 1024);

        Bytes output = new Bytes(geometry.numCylinders * geometry.numHeads * geometry.numSectors *
                geometry.sectorSize);
        ByteWriter bw = output.writer();

        int sectorFileOffset;
        for (int track = 0; track < geometry.numCylinders * geometry.numHeads; track++)
        {
            int side = (track < geometry.numCylinders) ? 0 : 1;
            for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
            {
                Sector sector = image.get(track % geometry.numCylinders, side, sectorId);
                if (sector != null)
                {
                    if (side == 0)
                    { /* Side 0 is from track 0-34 */
                        sectorFileOffset = track * trackSize + sectorId * geometry.sectorSize;
                    } else
                    { /* Side 1 is from track 70-35 */
                        sectorFileOffset = (geometry.sectorSize * geometry.numSectors *
                                geometry.numCylinders) + /* Skip over side 0 */
                                ((geometry.numCylinders - 1) - (track % geometry.numCylinders)) *
                                        (geometry.sectorSize * geometry.numSectors) +
                                (sectorId * geometry.sectorSize);
                    }
                    bw.seek(sectorFileOffset);
                    if ((geometry.sectorSize == 512) && (sector.data.size() == 256))
                    {
                        /* North Star DOS provided an upgrade path for disks
                         * formatted as single-density to hold double-density
                         * data without reformatting. In this case, the four
                         * directory blocks will be single-density but other
                         * areas of the disk are double-density. This cannot be
                         * accurately represented using a .nsi file, so in these
                         * cases, we pad the sector to 512-bytes, filling with
                         * spaces. */
                        if (!mixedDensity)
                        {
                            Logger.logf("Warning: Disk contains mixed " +
                                    "single/double-density sectors.");
                        }
                        mixedDensity = true;
                        bw.write(sector.data.slice(0, 256));
                        bw.pad(256, ' ');
                    } else
                    {
                        bw.write(sector.data.slice(0, geometry.sectorSize));
                    }
                }
            }
        }

        output.writeToFile(config.getFilename());
    }
}
