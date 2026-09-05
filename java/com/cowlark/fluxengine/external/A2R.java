package com.cowlark.fluxengine.external;

/**
 * A2R (AppleSauce) format definitions, ported from lib/external/a2r.h.
 *
 * <p>The canonical reference for the A2R format is:
 * https://applesaucefdc.com/a2r2-reference/ All data is stored little-endian.
 *
 * <p>Note: The first chunk begins at byte offset 8, not 12 as given in the
 * a2r2 reference version 2.0.1.
 */
public final class A2R
{
    public static final int CHUNK_INFO = 0x4F464E49;
    public static final int CHUNK_STRM = 0x4D525453;
    public static final int CHUNK_META = 0x4154454D;

    public static final int INFO_CHUNK_VERSION = 1;

    public static final int DISK_525 = 1;
    public static final int DISK_35 = 2;

    public static final int TIMING = 1;
    public static final int BITS = 2;
    public static final int XTIMING = 3;

    public static final int NS_PER_TICK = 125;

    public static final byte[] FILEHEADER =
            {'A', '2', 'R', '2', (byte) 0xff, (byte) 0x0a, (byte) 0x0d, (byte) 0x0a};

    private A2R()
    {
    }
}