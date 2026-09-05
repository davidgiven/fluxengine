package com.cowlark.fluxengine.arch.fb100;

/**
 * Constants for the FB100 format, ported from arch/fb100/fb100.h.
 */
public final class Fb100
{
    public static final int FB100_RECORD_SIZE = 0x516; /* bytes */
    public static final int FB100_ID_SIZE = 17;
    public static final int FB100_PAYLOAD_SIZE = 0x500;

    private Fb100()
    {
    }
}