package org.rob_smallshire.oaknut.adfs;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record AdfsGeometry(int tracks, int heads, int sectorsPerTrack, int bytesPerSector)
{
    public static AdfsGeometry of(AdfsFormat format)
    {
        return new AdfsGeometry(
                format.tracks,
                format.heads,
                format.sectorsPerTrack * format.sectorSize / 256,
                256);
    }

    public int cylinders()
    {
        return tracks;
    }

    public int sectorsPerCylinder()
    {
        return heads * sectorsPerTrack;
    }

    public int totalSectors()
    {
        return tracks * sectorsPerCylinder();
    }

    public int size()
    {
        return tracks * heads * sectorsPerTrack * bytesPerSector;
    }
}
