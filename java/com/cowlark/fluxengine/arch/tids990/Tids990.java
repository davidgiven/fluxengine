package com.cowlark.fluxengine.arch.tids990;

/**
 * Constants for the TI DS990 format, ported from arch/tids990/tids990.h.
 */
public final class Tids990
{
    public static final int TIDS990_PAYLOAD_SIZE = 288;                            /* bytes */
    public static final int TIDS990_SECTOR_RECORD_SIZE = 10;                       /* bytes */
    public static final int TIDS990_DATA_RECORD_SIZE = (TIDS990_PAYLOAD_SIZE + 4); /* bytes */

    private Tids990()
    {
    }
}