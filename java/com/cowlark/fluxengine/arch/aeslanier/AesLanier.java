package com.cowlark.fluxengine.arch.aeslanier;

/**
 * Constants for the AES Lanier format, ported from arch/aeslanier/aeslanier.h.
 */
public final class AesLanier
{
    public static final int AESLANIER_RECORD_SEPARATOR = 0x55555122;
    public static final int AESLANIER_SECTOR_LENGTH = 256;
    public static final int AESLANIER_RECORD_SIZE = AESLANIER_SECTOR_LENGTH + 5;

    private AesLanier()
    {
    }
}