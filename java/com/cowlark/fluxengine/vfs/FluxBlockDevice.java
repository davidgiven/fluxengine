package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Image;

public class FluxBlockDevice extends TrackedBlockDevice
{
    private final FilesystemOperation fso;

    public FluxBlockDevice(FilesystemOperation fso)
    {
        super(fso.getDiskLayout());
        this.fso = fso;
    }

    @Override
    protected void commitTrack(Image source, CylinderHead lch)
    {
        
    }

    @Override
    protected void populateTrack(Image destination, CylinderHead lch)
    {

    }
}
