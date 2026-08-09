package com.cowlark.fluxengine.arch.apple2;

import com.cowlark.fluxengine.apple2.Apple2EncoderProto;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import java.util.List;

/**
 * The Apple II encoder, ported from arch/apple2/encoder.cc.
 */
public class Apple2Encoder extends Encoder
{
    private static final int[] ENCODE_DATA_GCR = new int[64];

    static
    {
        ENCODE_DATA_GCR[0x00] = 0x96;
        ENCODE_DATA_GCR[0x01] = 0x97;
        ENCODE_DATA_GCR[0x02] = 0x9a;
        ENCODE_DATA_GCR[0x03] = 0x9b;
        ENCODE_DATA_GCR[0x04] = 0x9d;
        ENCODE_DATA_GCR[0x05] = 0x9e;
        ENCODE_DATA_GCR[0x06] = 0x9f;
        ENCODE_DATA_GCR[0x07] = 0xa6;
        ENCODE_DATA_GCR[0x08] = 0xa7;
        ENCODE_DATA_GCR[0x09] = 0xab;
        ENCODE_DATA_GCR[0x0a] = 0xac;
        ENCODE_DATA_GCR[0x0b] = 0xad;
        ENCODE_DATA_GCR[0x0c] = 0xae;
        ENCODE_DATA_GCR[0x0d] = 0xaf;
        ENCODE_DATA_GCR[0x0e] = 0xb2;
        ENCODE_DATA_GCR[0x0f] = 0xb3;
        ENCODE_DATA_GCR[0x10] = 0xb4;
        ENCODE_DATA_GCR[0x11] = 0xb5;
        ENCODE_DATA_GCR[0x12] = 0xb6;
        ENCODE_DATA_GCR[0x13] = 0xb7;
        ENCODE_DATA_GCR[0x14] = 0xb9;
        ENCODE_DATA_GCR[0x15] = 0xba;
        ENCODE_DATA_GCR[0x16] = 0xbb;
        ENCODE_DATA_GCR[0x17] = 0xbc;
        ENCODE_DATA_GCR[0x18] = 0xbd;
        ENCODE_DATA_GCR[0x19] = 0xbe;
        ENCODE_DATA_GCR[0x1a] = 0xbf;
        ENCODE_DATA_GCR[0x1b] = 0xcb;
        ENCODE_DATA_GCR[0x1c] = 0xcd;
        ENCODE_DATA_GCR[0x1d] = 0xce;
        ENCODE_DATA_GCR[0x1e] = 0xcf;
        ENCODE_DATA_GCR[0x1f] = 0xd3;
        ENCODE_DATA_GCR[0x20] = 0xd6;
        ENCODE_DATA_GCR[0x21] = 0xd7;
        ENCODE_DATA_GCR[0x22] = 0xd9;
        ENCODE_DATA_GCR[0x23] = 0xda;
        ENCODE_DATA_GCR[0x24] = 0xdb;
        ENCODE_DATA_GCR[0x25] = 0xdc;
        ENCODE_DATA_GCR[0x26] = 0xdd;
        ENCODE_DATA_GCR[0x27] = 0xde;
        ENCODE_DATA_GCR[0x28] = 0xdf;
        ENCODE_DATA_GCR[0x29] = 0xe5;
        ENCODE_DATA_GCR[0x2a] = 0xe6;
        ENCODE_DATA_GCR[0x2b] = 0xe7;
        ENCODE_DATA_GCR[0x2c] = 0xe9;
        ENCODE_DATA_GCR[0x2d] = 0xea;
        ENCODE_DATA_GCR[0x2e] = 0xeb;
        ENCODE_DATA_GCR[0x2f] = 0xec;
        ENCODE_DATA_GCR[0x30] = 0xed;
        ENCODE_DATA_GCR[0x31] = 0xee;
        ENCODE_DATA_GCR[0x32] = 0xef;
        ENCODE_DATA_GCR[0x33] = 0xf2;
        ENCODE_DATA_GCR[0x34] = 0xf3;
        ENCODE_DATA_GCR[0x35] = 0xf4;
        ENCODE_DATA_GCR[0x36] = 0xf5;
        ENCODE_DATA_GCR[0x37] = 0xf6;
        ENCODE_DATA_GCR[0x38] = 0xf7;
        ENCODE_DATA_GCR[0x39] = 0xf9;
        ENCODE_DATA_GCR[0x3a] = 0xfa;
        ENCODE_DATA_GCR[0x3b] = 0xfb;
        ENCODE_DATA_GCR[0x3c] = 0xfc;
        ENCODE_DATA_GCR[0x3d] = 0xfd;
        ENCODE_DATA_GCR[0x3e] = 0xfe;
        ENCODE_DATA_GCR[0x3f] = 0xff;
    }

    private final ConfigProto fullConfig;
    private final Apple2EncoderProto config;
    private int volumeId = 254;

    public Apple2Encoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getApple2();
    }

    private static int encodeDataGcr(int data)
    {
        if (data < 0 || data >= ENCODE_DATA_GCR.length)
            return -1;
        return ENCODE_DATA_GCR[data];
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        int bitsPerRevolution =
                (int) ((config.getRotationalPeriodMs() * 1e3) / config.getClockPeriodUs());

        Bits bits = new Bits(bitsPerRevolution);
        Bits.Cursor cursor = new Bits.Cursor(0);

        for (Sector sector : sectors)
            writeSector(bits, cursor, sector);

        if (cursor.get() >= bits.size())
            throw new FluxEngineException(
                    "track data overrun by " + (cursor.get() - bits.size()) + " bits");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits, (long) calculatePhysicalClockPeriod(
                        fullConfig,
                        config.getClockPeriodUs() * 1e3,
                        config.getRotationalPeriodMs() * 1e6));
        return fluxmap;
    }

    /* This is extremely inspired by the MESS implementation, written by Nathan
     * Woods and R. Belmont:
     * https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib
     * /formats/ap2_dsk.cpp
     * as well as Understanding the Apple II (1983) Chapter 9
     * https://archive.org/details/Understanding_the_Apple_II_1983_Quality_Software/page/n230
     * /mode/1up?view=theater
     */

    private void writeSector(Bits bits, Bits.Cursor cursor, Sector sector)
    {
        if ((sector.status == Sector.Status.OK) || (sector.status == Sector.Status.BAD_CHECKSUM))
        {
            // The special "FF40" sequence is used to synchronize the receiving
            // shift register. It's written as "1111 1111 00"; FF indicates the
            // 8 consecutive 1-bits, while "40" indicates the total number of
            // microseconds.
            // There is data to encode to disk.
            if ((sector.data.size() != Apple2.APPLE2_SECTOR_LENGTH))
                throw new FluxEngineException(
                        "unsupported sector size " + sector.data.size() + " --- you must pick 256");

            // Write address syncing leader : A sequence of "FF40"s; 5 of them
            // are said to suffice to synchronize the decoder.
            // "FF40" indicates that the actual data written is "1111
            // 1111 00" i.e., 8 1s and a total of 40 microseconds
            //
            // In standard formatting, the first logical sector apparently gets
            // extra padding.
            writeFf40(bits, cursor, sector.location.logicalSector() == 0 ? 32 : 8);

            int track = sector.location.logicalCylinder();
            if (sector.location.logicalHead() == 1)
                track += config.getSideOneTrackOffset();

            // Write address field: APPLE2_SECTOR_RECORD + sector identifier +
            // DE AA EB
            writeBits(bits, cursor, Apple2.APPLE2_SECTOR_RECORD, 24);
            writeGcr44(bits, cursor, volumeId);
            writeGcr44(bits, cursor, track);
            writeGcr44(bits, cursor, sector.location.logicalSector());
            writeGcr44(bits, cursor, volumeId ^ track ^ sector.location.logicalSector());
            writeBits(bits, cursor, 0xDEAAEB, 24);

            // Write data syncing leader: FF40 + APPLE2_DATA_RECORD + sector
            // data + sum + DE AA EB (+ mystery bits cut off of the scan?)
            writeFf40(bits, cursor, 8);
            writeBits(bits, cursor, Apple2.APPLE2_DATA_RECORD, 24);

            // Convert the sector data to GCR, append the checksum, and write it
            // out
            final int TWOBIT_COUNT = 0x56; // Size of the 'twobit' area at the start of the GCR data
            int checksum = 0;
            for (int i = 0; i < Apple2.APPLE2_ENCODED_SECTOR_LENGTH; i++)
            {
                int value;
                if (i >= TWOBIT_COUNT)
                {
                    value = sector.data.getByte(i - TWOBIT_COUNT) >> 2;
                } else
                {
                    int tmp = sector.data.getByte(i);
                    value = ((tmp & 1) << 1) | ((tmp & 2) >> 1);

                    tmp = sector.data.getByte(i + TWOBIT_COUNT);
                    value |= ((tmp & 1) << 3) | ((tmp & 2) << 1);

                    if (i + 2 * TWOBIT_COUNT < Apple2.APPLE2_SECTOR_LENGTH)
                    {
                        tmp = sector.data.getByte(i + 2 * TWOBIT_COUNT);
                        value |= ((tmp & 1) << 5) | ((tmp & 2) << 3);
                    }
                }
                checksum ^= value;
                writeGcr6(bits, cursor, checksum);
                checksum = value;
            }
            if (sector.status == Sector.Status.BAD_CHECKSUM)
                checksum ^= 0x3f;
            writeGcr6(bits, cursor, checksum);
            writeBits(bits, cursor, 0xDEAAEB, 24);
        }
    }

    private void writeBit(Bits bits, Bits.Cursor cursor, boolean val)
    {
        if (cursor.get() < bits.size())
            bits.setBit(cursor.get(), val);
        cursor.advance();
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, int data, int width)
    {
        for (int i = width; i-- != 0; )
            writeBit(bits, cursor, (data & (1 << i)) != 0);
    }

    private void writeGcr44(Bits bits, Bits.Cursor cursor, int value)
    {
        writeBits(bits, cursor, (value << 7) | value | 0xaaaa, 16);
    }

    private void writeGcr6(Bits bits, Bits.Cursor cursor, int value)
    {
        writeBits(bits, cursor, encodeDataGcr(value), 8);
    }

    private void writeFf40(Bits bits, Bits.Cursor cursor, int n)
    {
        for (; n-- != 0; )
            writeBits(bits, cursor, 0xff << 2, 10);
    }
}
