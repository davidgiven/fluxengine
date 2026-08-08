package com.cowlark.fluxengine.external;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;

/**
 * CRC helpers, ported from lib/core/crc.{h,cc}.
 */
public final class Crc
{
    public static final int CCITT_POLY = 0x1021;
    public static final int MODBUS_POLY = 0x8005;
    public static final int MODBUS_POLY_REF = 0xa001;
    public static final int BROTHER_POLY = 0x000201;

    private Crc()
    {
    }

    public static int crc16(int poly, int init, Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);

        int crc = init;
        while (!br.eof())
        {
            crc ^= br.read8() << 8;
            for (int i = 0; i < 8; i++)
                crc = (crc & 0x8000) != 0 ? ((crc << 1) ^ poly) : (crc << 1);
            crc &= 0xffff;
        }

        return crc;
    }

    public static int crc16(int poly, Bytes bytes)
    {
        return crc16(poly, 0xffff, bytes);
    }

    public static int crc16ref(int poly, int init, Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);

        int crc = init;
        while (!br.eof())
        {
            crc ^= br.read8();
            for (int i = 0; i < 8; i++)
                crc = (crc & 0x0001) != 0 ? ((crc >> 1) ^ poly) : (crc >> 1);
        }

        return crc;
    }

    public static int crc16ref(int poly, Bytes bytes)
    {
        return crc16ref(poly, 0xffff, bytes);
    }

    public static int sumBytes(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int sum = 0;
        while (!br.eof())
            sum += br.read8();
        return sum;
    }

    public static int xorBytes(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int result = 0;
        while (!br.eof())
            result ^= br.read8();
        return result;
    }
}