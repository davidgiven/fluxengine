package com.cowlark.fluxengine.arch.smaky6;

/**
 * Constants for the Smaky6 format, ported from arch/smaky6/smaky6.h.
 */
public final class Smaky6
{
    public static final int SMAKY6_SECTOR_SIZE = 256;
    public static final int SMAKY6_RECORD_SIZE = (1 + SMAKY6_SECTOR_SIZE + 1);

    private Smaky6()
    {
    }
}