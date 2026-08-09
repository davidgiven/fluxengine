package com.cowlark.fluxengine.arch.c64;

import com.cowlark.fluxengine.c64.Commodore64EncoderProto;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.Crc;
import java.util.List;

/**
 * The Commodore 64 encoder, ported from arch/c64/encoder.cc.
 */
public class Commodore64Encoder extends Encoder
{
    private static final int[] ENCODE_DATA_GCR = new int[16];

    static
    {
        ENCODE_DATA_GCR[0x0] = 0x0a;
        ENCODE_DATA_GCR[0x1] = 0x0b;
        ENCODE_DATA_GCR[0x2] = 0x12;
        ENCODE_DATA_GCR[0x3] = 0x13;
        ENCODE_DATA_GCR[0x4] = 0x0e;
        ENCODE_DATA_GCR[0x5] = 0x0f;
        ENCODE_DATA_GCR[0x6] = 0x16;
        ENCODE_DATA_GCR[0x7] = 0x17;
        ENCODE_DATA_GCR[0x8] = 0x09;
        ENCODE_DATA_GCR[0x9] = 0x19;
        ENCODE_DATA_GCR[0xa] = 0x1a;
        ENCODE_DATA_GCR[0xb] = 0x1b;
        ENCODE_DATA_GCR[0xc] = 0x0d;
        ENCODE_DATA_GCR[0xd] = 0x1d;
        ENCODE_DATA_GCR[0xe] = 0x1e;
        ENCODE_DATA_GCR[0xf] = 0x15;
    }

    private final ConfigProto fullConfig;
    private final Commodore64EncoderProto config;
    private int formatByte1;
    private int formatByte2;

    public Commodore64Encoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getC64();
    }

    private static int encodeDataGcr(int data)
    {
        if (data < 0 || data >= ENCODE_DATA_GCR.length)
            return -1;
        return ENCODE_DATA_GCR[data];
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

    /* See the big comment in the C++ file for the gory details of how 4
     * 8-bit bytes become five 8-bit GCR bytes; this encodes a single byte to
     * its 10-bit GCR form. */
    private static boolean[] encodeData(int input)
    {
        boolean[] output = new boolean[10];

        int lo = input >> 4; /* get the lo nibble */
        int hi = input & 15; /* get the hi nibble */

        int loGcr = encodeDataGcr(lo);
        int hiGcr = encodeDataGcr(hi);

        int b = 4;
        for (int i = 0; i < 10; i++)
        {
            if (i < 5)
            {
                output[4 - i] = (loGcr & 1) != 0;
                loGcr >>= 1;
            } else
            {
                output[i + b] = (hiGcr & 1) != 0;
                hiGcr >>= 1;
                b -= 2;
            }
        }
        return output;
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        /* The format ID Character # 1 and # 2 are in the .d64 image only
         * present in track 18 sector zero which contains the BAM info in byte
         * 162 and 163. it is written in every header of every sector and track.
         * headers are not stored in a d64 disk image so we have to get it from
         * track 18 which contains the BAM.
         */

        Sector sectorData = image.get(C64.C64_BAM_TRACK, 0, 0);
        if (sectorData != null)
        {
            ByteReader br = new ByteReader(sectorData.data);
            br.seek(162); /* goto position of the first Disk ID Byte */
            formatByte1 = br.read8();
            formatByte2 = br.read8();
        } else
        {
            formatByte1 = formatByte2 = 0;
        }

        double clockRateUs = C64.clockRateUsForTrack(ltl.logicalCylinder);
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

    private void writeSector(Bits bits, Bits.Cursor cursor, Sector sector)
    {
        if ((sector.status == Sector.Status.OK) || (sector.status == Sector.Status.BAD_CHECKSUM))
        {
            // There is data to encode to disk.
            if ((sector.data.size() != C64.C64_SECTOR_LENGTH))
                throw new FluxEngineException(
                        "unsupported sector size " + sector.data.size() + " --- you must pick 256");

            // 1. Write header Sync (not GCR)
            for (int i = 0; i < 6; i++)
                writeBits(bits, cursor, C64.C64_HEADER_DATA_SYNC, 1 * 8); /* sync */

            // 2. Write Header info 10 GCR bytes
            int encodedTrack = sector.location.logicalCylinder() + 1;
            int encodedSector = sector.location.logicalSector();
            int headerChecksum = (encodedTrack ^ encodedSector ^ formatByte1 ^ formatByte2);
            writeBits(bits, cursor, encodeData(C64.C64_HEADER_BLOCK_ID));
            writeBits(bits, cursor, encodeData(headerChecksum));
            writeBits(bits, cursor, encodeData(encodedSector));
            writeBits(bits, cursor, encodeData(encodedTrack));
            writeBits(bits, cursor, encodeData(formatByte2));
            writeBits(bits, cursor, encodeData(formatByte1));
            writeBits(bits, cursor, encodeData(C64.C64_PADDING));
            writeBits(bits, cursor, encodeData(C64.C64_PADDING));

            // 3. Write header GAP not GCR
            for (int i = 0; i < 9; i++)
                writeBits(bits, cursor, C64.C64_HEADER_GAP, 1 * 8); /* header gap */

            // 4. Write Data sync not GCR
            for (int i = 0; i < 6; i++)
                writeBits(bits, cursor, C64.C64_HEADER_DATA_SYNC, 1 * 8); /* sync */

            // 5. Write data block 325 GCR bytes
            writeBits(bits, cursor, encodeData(C64.C64_DATA_BLOCK_ID));
            int dataChecksum = Crc.xorBytes(sector.data);
            ByteReader br = new ByteReader(sector.data);
            for (int i = 0; i < C64.C64_SECTOR_LENGTH; i++)
                writeBits(bits, cursor, encodeData(br.read8()));
            writeBits(bits, cursor, encodeData(dataChecksum));
            writeBits(bits, cursor, encodeData(C64.C64_PADDING));
            writeBits(bits, cursor, encodeData(C64.C64_PADDING));

            // 6. Write inter-sector gap 9 - 12 bytes not GCR
            for (int i = 0; i < 9; i++)
                writeBits(bits, cursor, C64.C64_INTER_SECTOR_GAP, 1 * 8); /* sync */
        }
    }
}
