package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.google.common.collect.ImmutableCollection;
import java.util.Collection;

public class InMemoryBlockDevice extends TrackedBlockDevice
{
    private final Image image;

    public InMemoryBlockDevice(DiskLayout diskLayout, Image image)
    {
        super(diskLayout);
        image.addMissingSectors(diskLayout, true);
        this.image = image;
    }

    @Override
    protected void populateTracks(Image destination, ImmutableCollection<CylinderHead> lchs)
    {
        for (CylinderHead lch : lchs)
            copySectors(image, destination, lch);
    }

    @Override
    protected void commitTracks(Image source, ImmutableCollection<CylinderHead> lchs)
    {
        for (CylinderHead lch : lchs)
            copySectors(source, image, lch);
    }
}
