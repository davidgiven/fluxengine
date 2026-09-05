package com.cowlark.fluxengine.arch.micropolis;

/**
 * Constants for the Micropolis format, ported from arch/micropolis/micropolis.h.
 */
public final class Micropolis
{
    public static final int MICROPOLIS_PAYLOAD_SIZE = (256);
    public static final int MICROPOLIS_HEADER_SIZE = (1 + 2 + 10);
    public static final int MICROPOLIS_ENCODED_SECTOR_SIZE =
            (MICROPOLIS_HEADER_SIZE + MICROPOLIS_PAYLOAD_SIZE + 6);

    private Micropolis()
    {
    }
}