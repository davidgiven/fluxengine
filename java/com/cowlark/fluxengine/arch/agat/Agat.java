package com.cowlark.fluxengine.arch.agat;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;

/**
 * Constants and helpers for the Agat format, ported from
 * arch/agat/agat.h and arch/agat/agat.cc.
 */
public final class Agat
{
    public static final int AGAT_SECTOR_SIZE = 256;

    public static final long SECTOR_ID = 0x8924555549111444L;
    public static final long DATA_ID = 0x8924555514444911L;

    private Agat()
    {
    }

    public static int agatChecksum(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int checksum = 0;

        while (!br.eof())
        {
            int b = br.read8();
            if (checksum > 0xff)
                checksum = (checksum + 1) & 0xff;

            checksum += b;
        }

        return checksum & 0xff;
    }
}