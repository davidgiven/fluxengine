package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;

/**
 * Writes a raw (non-interleaved) sector image, ported from
 * lib/imagewriter/imgimagewriter.cc.
 */
public class ImgImageWriter extends ImageWriter
{
    private final ConfigProto config;

    /* The img writer needs the full config to determine the layout; created
     * via ImageWriter.create(ConfigProto). */
    public ImgImageWriter(ImageWriterProto writerConfig, ConfigProto config)
    {
        super(writerConfig);
        this.config = config;
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();

        int tracks = config.getLayout().hasTracks() ?
                config.getLayout().getTracks() :
                geometry.numCylinders;
        int sides =
                config.getLayout().hasSides() ? config.getLayout().getSides() : geometry.numHeads;

        DiskLayout diskLayout = new DiskLayout(config);
        boolean inFilesystemOrder = getWriterConfig().getImg().getFilesystemSectorOrder();

        Bytes output = new Bytes();
        ByteWriter bw = output.writer();

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
                Sector sector =
                        image.get(logicalLocation.cylinder(), logicalLocation.head(), sectorId);
                if (sector != null)
                    bw.write(sector.data.slice(0, ltl.sectorSize));
                else
                    bw.pad(ltl.sectorSize);
            }
        }

        output.writeToFile(getWriterConfig().getFilename());

        System.out.printf(
                "IMG: wrote %d tracks, %d sides, %d kB total to %s%n",
                tracks,
                sides,
                output.size() / 1024,
                getWriterConfig().getFilename());
    }
}
