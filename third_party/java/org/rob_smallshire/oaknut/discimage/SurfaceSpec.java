package org.rob_smallshire.oaknut.discimage;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Physical mapping of one disc side into its image. */
public record SurfaceSpec(
    int tracks, int sectorsPerTrack, int bytesPerSector, int trackZeroOffset, int trackStride)
{
    public SurfaceSpec
    {
        if (tracks <= 0 || sectorsPerTrack <= 0 || bytesPerSector <= 0 || trackZeroOffset < 0 ||
            trackStride < sectorsPerTrack * bytesPerSector)
            throw new IllegalArgumentException("Invalid surface geometry");
    }

    public int sectors()
    { return Math.multiplyExact(tracks, sectorsPerTrack); }
    public int bytes()
    { return Math.multiplyExact(sectors(), bytesPerSector); }

    public int offset(int sector)
    {
        java.util.Objects.checkIndex(sector, sectors());
        return trackZeroOffset + sector / sectorsPerTrack * trackStride +
            sector % sectorsPerTrack * bytesPerSector;
    }
}
