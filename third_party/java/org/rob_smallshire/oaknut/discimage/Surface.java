package org.rob_smallshire.oaknut.discimage;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record Surface(DiscImage image, int side)
{
    public SurfaceSpec spec()
    {
        return image.surfaces().get(side);
    }

    public SectorsView sectorRange(int start, int count)
    {
        return image.sectors(side, start, count);
    }
}
