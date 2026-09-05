package com.cowlark.fluxengine.arch.apple2;

/**
 * Constants for the Apple II format, ported from arch/apple2/apple2.h.
 */
public final class Apple2
{
    public static final int APPLE2_SECTOR_RECORD = 0xd5aa96;
    public static final int APPLE2_DATA_RECORD = 0xd5aaad;

    public static final int APPLE2_SECTOR_LENGTH = 256;
    public static final int APPLE2_ENCODED_SECTOR_LENGTH = 342;

    public static final int APPLE2_SECTORS = 16;

    private Apple2()
    {
    }
}