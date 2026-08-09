package com.cowlark.fluxengine.arch.macintosh;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.macintosh.MacintoshEncoderProto;
import java.util.List;

/**
 * The Macintosh encoder, ported from arch/macintosh/encoder.cc.
 */
public class MacintoshEncoder extends Encoder
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
    private final MacintoshEncoderProto config;

    public MacintoshEncoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getMacintosh();
    }

    private static int encodeDataGcr(int data)
    {
        if (data < 0 || data >= ENCODE_DATA_GCR.length)
            return -1;
        return ENCODE_DATA_GCR[data];
    }

    private static double clockRateUsForTrack(int track)
    {
        if (track < 16)
            return 2.63;
        if (track < 32)
            return 2.89;
        if (track < 48)
            return 3.20;
        if (track < 64)
            return 3.57;
        return 3.98;
    }

    @SuppressWarnings("unused")
    private static int sectorsForTrack(int track)
    {
        if (track < 16)
            return 12;
        if (track < 32)
            return 11;
        if (track < 48)
            return 10;
        if (track < 64)
            return 9;
        return 8;
    }

    /* This is extremely inspired by the MESS implementation, written by Nathan
     * Woods and R. Belmont:
     * https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib
     * /formats/ap_dsk35.cpp
     */
    private static Bytes encodeCrazyData(Bytes input)
    {
        Bytes output = new Bytes(0);
        ByteWriter bw = output.writer();
        ByteReader br = new ByteReader(input);

        final int LOOKUP_LEN = Macintosh.MAC_SECTOR_LENGTH / 3;

        int[] b1 = new int[LOOKUP_LEN + 1];
        int[] b2 = new int[LOOKUP_LEN + 1];
        int[] b3 = new int[LOOKUP_LEN + 1];

        int c1 = 0;
        int c2 = 0;
        int c3 = 0;
        for (int j = 0; ; j++)
        {
            c1 = (c1 & 0xff) << 1;
            if ((c1 & 0x0100) != 0)
                c1++;

            int val = br.read8();
            c3 += val;
            if ((c1 & 0x0100) != 0)
            {
                c3++;
                c1 &= 0xff;
            }
            b1[j] = (val ^ c1) & 0xff;

            val = br.read8();
            c2 += val;
            if (c3 > 0xff)
            {
                c2++;
                c3 &= 0xff;
            }
            b2[j] = (val ^ c3) & 0xff;

            if (br.pos() == 524)
                break;

            val = br.read8();
            c1 += val;
            if (c2 > 0xff)
            {
                c1++;
                c2 &= 0xff;
            }
            b3[j] = (val ^ c2) & 0xff;
        }
        int c4 = ((c1 & 0xc0) >> 6) | ((c2 & 0xc0) >> 4) | ((c3 & 0xc0) >> 2);
        b3[LOOKUP_LEN] = 0;

        for (int i = 0; i <= LOOKUP_LEN; i++)
        {
            int w1 = b1[i] & 0x3f;
            int w2 = b2[i] & 0x3f;
            int w3 = b3[i] & 0x3f;
            int w4 = (b1[i] & 0xc0) >> 2;
            w4 |= (b2[i] & 0xc0) >> 4;
            w4 |= (b3[i] & 0xc0) >> 6;

            bw.write8(w4);
            bw.write8(w1);
            bw.write8(w2);

            if (i != LOOKUP_LEN)
                bw.write8(w3);
        }

        bw.write8(c4 & 0x3f);
        bw.write8(c3 & 0x3f);
        bw.write8(c2 & 0x3f);
        bw.write8(c1 & 0x3f);

        return output;
    }

    private static void writeBits(Bits bits, Bits.Cursor cursor, boolean[] src)
    {
        for (boolean bit : src)
        {
            if (cursor.get() < bits.size())
                bits.setBit(cursor.get(), bit);
            cursor.advance();
        }
    }

    private static void writeBits(Bits bits, Bits.Cursor cursor, long data, int width)
    {
        cursor.advance(width);
        for (int i = 0; i < width; i++)
        {
            int pos = cursor.get() - i - 1;
            if (pos < bits.size())
                bits.setBit(pos, (data & 1) != 0);
            data >>= 1;
        }
    }

    private static int encodeSide(int track, int side)
    {
        /* Mac disks, being weird, use the side byte to encode both the side (in
         * bit 5) and also whether we're above track 0x3f (in bit 0).
         */

        return (side != 0 ? 0x20 : 0x00) | ((track > 0x3f) ? 0x01 : 0x00);
    }

    private static void writeSector(Bits bits, Bits.Cursor cursor, Sector sector)
    {
        if ((sector.data.size() != 512) && (sector.data.size() != 524))
            throw new FluxEngineException("unsupported sector size --- you must pick 512 or 524");

        writeBits(bits, cursor, 0xff, 1 * 8); /* pad byte */
        for (int i = 0; i < 7; i++)
            writeBits(bits, cursor, 0xff3fcff3fcffL, 6 * 8); /* sync */
        writeBits(bits, cursor, Macintosh.MAC_SECTOR_RECORD, 3 * 8);

        int encodedTrack = sector.location.logicalCylinder() & 0x3f;
        int encodedSector = sector.location.logicalSector();
        int encodedSide =
                encodeSide(sector.location.logicalCylinder(), sector.location.logicalHead());
        int formatByte = Macintosh.MAC_FORMAT_BYTE;
        int headerChecksum = (encodedTrack ^ encodedSector ^ encodedSide ^ formatByte) & 0x3f;

        writeBits(bits, cursor, encodeDataGcr(encodedTrack), 1 * 8);
        writeBits(bits, cursor, encodeDataGcr(encodedSector), 1 * 8);
        writeBits(bits, cursor, encodeDataGcr(encodedSide), 1 * 8);
        writeBits(bits, cursor, encodeDataGcr(formatByte), 1 * 8);
        writeBits(bits, cursor, encodeDataGcr(headerChecksum), 1 * 8);

        writeBits(bits, cursor, 0xdeaaff, 3 * 8);
        writeBits(bits, cursor, 0xff3fcff3fcffL, 6 * 8); /* sync */
        writeBits(bits, cursor, Macintosh.MAC_DATA_RECORD, 3 * 8);
        writeBits(bits, cursor, encodeDataGcr(sector.location.logicalSector()), 1 * 8);

        Bytes wireData = sector.data.slice(512, 12).concat(sector.data.slice(0, 512));
        Bytes crazy = encodeCrazyData(wireData);
        for (int i = 0; i < crazy.size(); i++)
            writeBits(bits, cursor, encodeDataGcr(crazy.getByte(i) & 0xff), 1 * 8);

        writeBits(bits, cursor, 0xdeaaff, 3 * 8);
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        double clockRateUs = clockRateUsForTrack(ltl.logicalCylinder);
        int bitsPerRevolution = (int) (200000.0 / clockRateUs);
        Bits bits = new Bits(bitsPerRevolution);
        Bits.Cursor cursor = new Bits.Cursor(0);

        bits.fillBitmapTo(
                cursor,
                (int) (config.getPostIndexGapUs() / clockRateUs),
                new boolean[]{true, false});

        for (Sector sector : sectors)
            writeSector(bits, cursor, sector);

        if (cursor.get() >= bits.size())
            throw new FluxEngineException(
                    "track data overrun by " + (cursor.get() - bits.size()) + " bits");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits,
                (long) calculatePhysicalClockPeriod(fullConfig, clockRateUs * 1e3, 200e6));
        return fluxmap;
    }
}
