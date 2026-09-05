package com.cowlark.fluxengine.arch.c64;

/**
 * Constants for the Commodore 64 format, ported from arch/c64/c64.h.
 * <p>
 * Source: http://www.unusedino.de/ec64/technical/formats/g64.html
 * 1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR)
 * 2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
 * 3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
 * 4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR)
 * 5. Data block        55...4A (325 GCR bytes)
 * 6. Inter-sector gap  55 55 55 55...55 55 (4 to 12 bytes, never read)
 * 1. Header sync       (SYNC for the next sector)
 */
public final class C64
{
    public static final int C64_SECTOR_RECORD = 0xffd49;
    public static final int C64_DATA_RECORD = 0xffd57;
    public static final int C64_SECTOR_LENGTH = 256;

    public static final int C64_HEADER_DATA_SYNC = 0xFF;
    public static final int C64_HEADER_BLOCK_ID = 0x08;
    public static final int C64_DATA_BLOCK_ID = 0x07;
    public static final int C64_HEADER_GAP = 0x55;
    public static final int C64_INTER_SECTOR_GAP = 0x55;
    public static final int C64_PADDING = 0x0F;

    public static final int C64_TRACKS_PER_DISK = 40;
    public static final int C64_BAM_TRACK = 17;

    private C64()
    {
    }

    /*
     *   Track   Sectors/track   # Sectors   Storage in Bytes   Clock rate
     *   -----   -------------   ---------   ----------------   ----------
     *    1-17        21            357           7820             3.25
     *   18-24        19            133           7170             3.5
     *   25-30        18            108           6300             3.75
     *   31-40(*)     17             85           6020             4
     *                              ---
     *                              683 (for a 35 track image)
     *
     * The clock rate is normalised for a 200ms drive.
     */
    public static double clockRateUsForTrack(int track)
    {
        if (track < 17)
            return 26.0 / 8.0;
        if (track < 24)
            return 28.0 / 8.0;
        if (track < 30)
            return 30.0 / 8.0;
        return 32.0 / 8.0;
    }
}