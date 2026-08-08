package com.cowlark.fluxengine.arch.macintosh;

/**
 * Constants for the Macintosh format, ported from arch/macintosh/macintosh.h.
 */
public final class Macintosh
{
    public static final int MAC_SECTOR_RECORD = 0xd5aa96; /* 1101 0101 1010 1010 1001 0110 */
    public static final int MAC_DATA_RECORD = 0xd5aaad;   /* 1101 0101 1010 1010 1010 1101 */

    public static final int MAC_SECTOR_LENGTH = 524; /* yes, really */
    public static final int MAC_ENCODED_SECTOR_LENGTH = 703;
    public static final int MAC_FORMAT_BYTE = 0x22;

    public static final int MAC_TRACKS_PER_DISK = 80;

    private Macintosh()
    {
    }
}