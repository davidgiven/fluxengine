package com.cowlark.fluxengine.arch.brother;

import com.cowlark.fluxengine.brother.BrotherEncoderProto;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.Crc;
import java.util.List;

/**
 * The Brother encoder, ported from arch/brother/encoder.cc.
 */
public class BrotherEncoder extends Encoder
{
    private static final int[] ENCODE_HEADER_GCR = new int[78];
    private static final int[] ENCODE_DATA_GCR = new int[32];

    static
    {
        ENCODE_HEADER_GCR[0] = 0xDFB5;
        ENCODE_HEADER_GCR[1] = 0x5B6F;
        ENCODE_HEADER_GCR[2] = 0x7DF7;
        ENCODE_HEADER_GCR[3] = 0xBFD5;
        ENCODE_HEADER_GCR[4] = 0xF57F;
        ENCODE_HEADER_GCR[5] = 0x6D5D;
        ENCODE_HEADER_GCR[6] = 0xAFEB;
        ENCODE_HEADER_GCR[7] = 0xDDB7;
        ENCODE_HEADER_GCR[8] = 0x5775;
        ENCODE_HEADER_GCR[9] = 0x7BFB;
        ENCODE_HEADER_GCR[10] = 0xBDD7;
        ENCODE_HEADER_GCR[11] = 0xEFAB;
        ENCODE_HEADER_GCR[12] = 0x6B5F;
        ENCODE_HEADER_GCR[13] = 0xADED;
        ENCODE_HEADER_GCR[14] = 0xDBBB;
        ENCODE_HEADER_GCR[15] = 0x5577;
        ENCODE_HEADER_GCR[16] = 0x77DB;
        ENCODE_HEADER_GCR[17] = 0xBBAD;
        ENCODE_HEADER_GCR[18] = 0xED6B;
        ENCODE_HEADER_GCR[19] = 0x5FEF;
        ENCODE_HEADER_GCR[20] = 0xABBD;
        ENCODE_HEADER_GCR[21] = 0xD77B;
        ENCODE_HEADER_GCR[22] = 0xFB57;
        ENCODE_HEADER_GCR[23] = 0x75DD;
        ENCODE_HEADER_GCR[24] = 0xB7AF;
        ENCODE_HEADER_GCR[25] = 0xEB6D;
        ENCODE_HEADER_GCR[26] = 0x5DF5;
        ENCODE_HEADER_GCR[27] = 0x7FBF;
        ENCODE_HEADER_GCR[28] = 0xD57D;
        ENCODE_HEADER_GCR[29] = 0xF75B;
        ENCODE_HEADER_GCR[30] = 0x6FDF;
        ENCODE_HEADER_GCR[31] = 0xB5B5;
        ENCODE_HEADER_GCR[32] = 0xDF6F;
        ENCODE_HEADER_GCR[33] = 0x5BF7;
        ENCODE_HEADER_GCR[34] = 0x7DD5;
        ENCODE_HEADER_GCR[35] = 0xBF7F;
        ENCODE_HEADER_GCR[36] = 0xF55D;
        ENCODE_HEADER_GCR[37] = 0x6DEB;
        ENCODE_HEADER_GCR[38] = 0xAFB7;
        ENCODE_HEADER_GCR[39] = 0xDD75;
        ENCODE_HEADER_GCR[40] = 0x57FB;
        ENCODE_HEADER_GCR[41] = 0x7BD7;
        ENCODE_HEADER_GCR[42] = 0xBDAB;
        ENCODE_HEADER_GCR[43] = 0xEF5F;
        ENCODE_HEADER_GCR[44] = 0x6BED;
        ENCODE_HEADER_GCR[45] = 0xADBB;
        ENCODE_HEADER_GCR[46] = 0xDB77;
        ENCODE_HEADER_GCR[47] = 0xBB55;
        ENCODE_HEADER_GCR[48] = 0xEDDB;
        ENCODE_HEADER_GCR[49] = 0x5FAD;
        ENCODE_HEADER_GCR[50] = 0xAB6B;
        ENCODE_HEADER_GCR[51] = 0xD7EF;
        ENCODE_HEADER_GCR[52] = 0xFBBD;
        ENCODE_HEADER_GCR[53] = 0x757B;
        ENCODE_HEADER_GCR[54] = 0xB757;
        ENCODE_HEADER_GCR[55] = 0xEBDD;
        ENCODE_HEADER_GCR[56] = 0x5DAF;
        ENCODE_HEADER_GCR[57] = 0x7F6D;
        ENCODE_HEADER_GCR[58] = 0xD5F5;
        ENCODE_HEADER_GCR[59] = 0xF7BF;
        ENCODE_HEADER_GCR[60] = 0x6F7D;
        ENCODE_HEADER_GCR[61] = 0xB55B;
        ENCODE_HEADER_GCR[62] = 0xDFDF;
        ENCODE_HEADER_GCR[63] = 0x5BB5;
        ENCODE_HEADER_GCR[64] = 0x7D6F;
        ENCODE_HEADER_GCR[65] = 0xBFF7;
        ENCODE_HEADER_GCR[66] = 0xF5D5;
        ENCODE_HEADER_GCR[67] = 0x6D7F;
        ENCODE_HEADER_GCR[68] = 0xAF5D;
        ENCODE_HEADER_GCR[69] = 0xDDEB;
        ENCODE_HEADER_GCR[70] = 0x57B7;
        ENCODE_HEADER_GCR[71] = 0x7B75;
        ENCODE_HEADER_GCR[72] = 0xBDFB;
        ENCODE_HEADER_GCR[73] = 0xEFD7;
        ENCODE_HEADER_GCR[74] = 0x6BAB;
        ENCODE_HEADER_GCR[75] = 0xAD5F;
        ENCODE_HEADER_GCR[76] = 0xDBED;
        ENCODE_HEADER_GCR[77] = 0x55BB;

        ENCODE_DATA_GCR[0] = 0x55;
        ENCODE_DATA_GCR[1] = 0x57;
        ENCODE_DATA_GCR[2] = 0x5b;
        ENCODE_DATA_GCR[3] = 0x5d;
        ENCODE_DATA_GCR[4] = 0x5f;
        ENCODE_DATA_GCR[5] = 0x6b;
        ENCODE_DATA_GCR[6] = 0x6d;
        ENCODE_DATA_GCR[7] = 0x6f;
        ENCODE_DATA_GCR[8] = 0x75;
        ENCODE_DATA_GCR[9] = 0x77;
        ENCODE_DATA_GCR[10] = 0x7b;
        ENCODE_DATA_GCR[11] = 0x7d;
        ENCODE_DATA_GCR[12] = 0x7f;
        ENCODE_DATA_GCR[13] = 0xab;
        ENCODE_DATA_GCR[14] = 0xad;
        ENCODE_DATA_GCR[15] = 0xaf;
        ENCODE_DATA_GCR[16] = 0xb5;
        ENCODE_DATA_GCR[17] = 0xb7;
        ENCODE_DATA_GCR[18] = 0xbb;
        ENCODE_DATA_GCR[19] = 0xbd;
        ENCODE_DATA_GCR[20] = 0xbf;
        ENCODE_DATA_GCR[21] = 0xd5;
        ENCODE_DATA_GCR[22] = 0xd7;
        ENCODE_DATA_GCR[23] = 0xdb;
        ENCODE_DATA_GCR[24] = 0xdd;
        ENCODE_DATA_GCR[25] = 0xdf;
        ENCODE_DATA_GCR[26] = 0xeb;
        ENCODE_DATA_GCR[27] = 0xed;
        ENCODE_DATA_GCR[28] = 0xef;
        ENCODE_DATA_GCR[29] = 0xf5;
        ENCODE_DATA_GCR[30] = 0xf7;
        ENCODE_DATA_GCR[31] = 0xfb;
    }

    private final BrotherEncoderProto config;

    public BrotherEncoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getBrother();
    }

    private static int encodeHeaderGcr(int word)
    {
        if (word < 0 || word >= ENCODE_HEADER_GCR.length)
            return -1;
        return ENCODE_HEADER_GCR[word];
    }

    private static int encodeDataGcr(int data)
    {
        if (data < 0 || data >= ENCODE_DATA_GCR.length)
            return -1;
        return ENCODE_DATA_GCR[data];
    }

    private static void writeBits(Bits bits, Bits.Cursor cursor, int data, int width)
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

    private static void writeSectorHeader(Bits bits, Bits.Cursor cursor, int track, int sector)
    {
        writeBits(bits, cursor, 0xffffffff, 31);
        writeBits(bits, cursor, Brother.BROTHER_SECTOR_RECORD, 32);
        writeBits(bits, cursor, encodeHeaderGcr(track), 16);
        writeBits(bits, cursor, encodeHeaderGcr(sector), 16);
        writeBits(bits, cursor, encodeHeaderGcr(0x2f), 16);
    }

    private static void writeSectorData(Bits bits, Bits.Cursor cursor, Bytes data)
    {
        writeBits(bits, cursor, 0xffffffff, 32);
        writeBits(bits, cursor, Brother.BROTHER_DATA_RECORD, 32);

        if (data.size() != Brother.BROTHER_DATA_RECORD_PAYLOAD)
            throw new FluxEngineException("unsupported sector size");

        int[] fifo = {0};
        int[] width = {0};

        /* Consume 5-bit quintets from a 16-bit fifo fed by 8-bit bytes. */
        java.util.function.IntConsumer writeByte = (byte_) -> {
            fifo[0] = (fifo[0] | (byte_ << (8 - width[0]))) & 0xffff;
            width[0] += 8;

            while (width[0] >= 5)
            {
                int quintet = fifo[0] >> 11;
                fifo[0] = (fifo[0] << 5) & 0xffff;
                width[0] -= 5;

                writeBits(bits, cursor, encodeDataGcr(quintet), 8);
            }
        };

        for (int i = 0; i < data.size(); i++)
            writeByte.accept(data.getByte(i));

        int realCrc = Crc.crcbrother(data);
        writeByte.accept(realCrc >> 16);
        writeByte.accept(realCrc >> 8);
        writeByte.accept(realCrc);
        writeByte.accept(0x58); /* magic */
        writeByte.accept(0xd4);
        while (width[0] != 0)
            writeByte.accept(0);
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        int bitsPerRevolution = (int) (200000.0 / config.getClockRateUs());
        Bits bits = new Bits(bitsPerRevolution);
        Bits.Cursor cursor = new Bits.Cursor(0);

        int sectorCount = 0;
        for (Sector sectorData : sectors)
        {
            double headerMs =
                    config.getPostIndexGapMs() + sectorCount * config.getSectorSpacingMs();
            int headerCursor = (int) (headerMs * 1e3 / config.getClockRateUs());
            double dataMs = headerMs + config.getPostHeaderSpacingMs();
            int dataCursor = (int) (dataMs * 1e3 / config.getClockRateUs());

            bits.fillBitmapTo(cursor, headerCursor, new boolean[]{true, false});
            writeSectorHeader(
                    bits,
                    cursor,
                    sectorData.location.logicalCylinder(),
                    sectorData.location.logicalSector());
            bits.fillBitmapTo(cursor, dataCursor, new boolean[]{true, false});
            writeSectorData(bits, cursor, sectorData.data);

            sectorCount++;
        }

        if (cursor.get() >= bits.size())
            throw new FluxEngineException("track data overrun");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(bits, (long) (config.getClockRateUs() * 1e3));
        return fluxmap;
    }
}
