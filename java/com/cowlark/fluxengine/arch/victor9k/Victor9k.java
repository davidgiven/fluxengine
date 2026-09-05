package com.cowlark.fluxengine.arch.victor9k;

/**
 * Constants for the Victor 9k format, ported from arch/victor9k/victor9k.h.
 */
public final class Victor9k
{
    /* ... 1101 0101 0111
     *       ^^ ^^^^ ^^^^ ten bit IO byte */
    public static final int VICTOR9K_SECTOR_RECORD = 0xfffffd57;
    public static final int VICTOR9K_HEADER_ID = 0x7;

    /* ... 1101 0100 1001
     *       ^^ ^^^^ ^^^^ ten bit IO byte */
    public static final int VICTOR9K_DATA_RECORD = 0xfffffd49;
    public static final int VICTOR9K_DATA_ID = 0x8;

    public static final int VICTOR9K_SECTOR_LENGTH = 512;

    private Victor9k()
    {
    }
}