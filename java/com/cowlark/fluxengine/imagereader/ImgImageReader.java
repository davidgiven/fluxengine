package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.LayoutProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Reads a raw (non-interleaved) sector image, ported from
 * lib/imagereader/imgimagereader.cc.
 */
public class ImgImageReader extends ImageReader
{
    public ImgImageReader(ImageReaderProto config, ConfigProto fullConfig)
    {
        super(config, fullConfig);
    }

    @Override
    public Image readImage()
    {
        LayoutProto layout = fullConfig.getLayout();
        if (!layout.hasTracks() || !layout.hasSides())
            throw new FluxEngineException("IMG: bad configuration; did you remember to set the " +
                    "tracks, sides and trackdata fields in the layout?");

        DiskLayout diskLayout = new DiskLayout(fullConfig);
        boolean inFilesystemOrder = config.getImg().getFilesystemSectorOrder();
        Image image = new Image();

        try (InputStream inputFile = Files.newInputStream(Path.of(config.getFilename())))
        {
            Iterable<CylinderHead> locations = inFilesystemOrder ?
                    diskLayout.logicalLocationsInFilesystemOrder :
                    diskLayout.logicalLocations;
            for (CylinderHead logicalLocation : locations)
            {
                LogicalTrackLayout ltl = diskLayout.layoutByLogicalLocation.get(logicalLocation);

                Iterable<Integer> sectorOrder =
                        inFilesystemOrder ? ltl.filesystemSectorOrder : ltl.naturalSectorOrder;
                for (int sectorId : sectorOrder)
                {
                    byte[] buf = new byte[ltl.sectorSize];
                    int read = inputFile.read(buf);
                    if (read == -1)
                        break;

                    Sector sector =
                            image.put(logicalLocation.cylinder(), logicalLocation.head(), sectorId);
                    sector.status = Sector.Status.OK;
                    sector.data = new Bytes(buf);
                }
            }
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open input file");
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf("IMG: read " + geometry.numCylinders + " tracks, " + geometry.numHeads +
                " sides, " + geometry.totalBytes / 1024 + " kB total from " + config.getFilename());
        return image;
    }
}
