package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;

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
    protected void populateTrack(Image destination, CylinderHead lch)
    {
        copySectors(image, destination, lch);
    }

    @Override
    protected void commitTrack(Image source, CylinderHead lch)
    {
        copySectors(source, image, lch);
    }
}
