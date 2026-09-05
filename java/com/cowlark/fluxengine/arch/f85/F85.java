package com.cowlark.fluxengine.arch.f85;

/**
 * Constants for the Durango F85 format, ported from arch/f85/f85.h.
 */
public final class F85
{
    public static final int F85_SECTOR_RECORD = 0xffffce; /* 1111 1111 1111 1111 1100 1110 */
    public static final int F85_DATA_RECORD = 0xffffcb;   /* 1111 1111 1111 1111 1100 1101 */
    public static final int F85_SECTOR_LENGTH = 512;

    private F85()
    {
    }
}