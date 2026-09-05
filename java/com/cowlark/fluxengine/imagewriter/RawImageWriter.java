package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Record;
import com.cowlark.fluxengine.data.Sector;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Writes a raw (flux-level) image, ported from
 * lib/imagewriter/rawimagewriter.cc.
 */
public class RawImageWriter extends ImageWriter
{
    public RawImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();

        int trackSize = geometry.numSectors * geometry.sectorSize;

        if (geometry.numCylinders * trackSize == 0)
        {
            Logger.logf("RAW: no sectors in output; skipping image file generation.");
            return;
        }

        Logger.logf("RAW: writing %d tracks, %d sides", geometry.numCylinders, geometry.numHeads);

        Bytes output = new Bytes();

        for (int track = 0; track < geometry.numCylinders * geometry.numHeads; track++)
        {
            int side = (track < geometry.numCylinders) ? 0 : 1;

            List<Record> records = new ArrayList<>();
            for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
            {
                Sector sector = image.get(track % geometry.numCylinders, side, sectorId);
                if (sector != null)
                    records.addAll(sector.records);
            }

            records.sort(Comparator.comparingDouble(r -> r.startTimeNs));

            for (Record record : records)
            {
                output = output.concat(record.rawData);
                output = output.concat(new Bytes(3));
            }
            output = output.concat(new Bytes(1));
        }

        output.writeToFile(config.getFilename());
    }
}
