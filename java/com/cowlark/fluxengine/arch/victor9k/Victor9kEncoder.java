package com.cowlark.fluxengine.arch.victor9k;

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
import com.cowlark.fluxengine.victor9k.Victor9kEncoderProto;
import java.util.List;

/**
 * The Victor 9k encoder, ported from arch/victor9k/encoder.cc.
 */
public class Victor9kEncoder extends Encoder
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

    private final Victor9kEncoderProto config;
    private final boolean[] lastBit = new boolean[1];

    public Victor9kEncoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getVictor9K();
    }

    private static int encodeDataGcr(int data)
    {
        data &= 0x0f;
        return ENCODE_DATA_GCR[data];
    }

    private void writeZeroBits(Bits bits, Bits.Cursor cursor, int count)
    {
        while (count-- != 0)
        {
            if (cursor.get() < bits.size())
            {
                lastBit[0] = false;
                bits.setBit(cursor.get(), false);
            }
            cursor.advance();
        }
    }

    private void writeOneBits(Bits bits, Bits.Cursor cursor, int count)
    {
        while (count-- != 0)
        {
            if (cursor.get() < bits.size())
            {
                lastBit[0] = true;
                bits.setBit(cursor.get(), true);
            }
            cursor.advance();
        }
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, boolean[] src)
    {
        for (boolean bit : src)
        {
            if (cursor.get() < bits.size())
            {
                lastBit[0] = bit;
                bits.setBit(cursor.get(), bit);
            }
            cursor.advance();
        }
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, long data, int width)
    {
        cursor.advance(width);
        lastBit[0] = (data & 1) != 0;
        for (int i = 0; i < width; i++)
        {
            int pos = cursor.get() - i - 1;
            if (pos < bits.size())
                bits.setBit(pos, (data & 1) != 0);
            data >>= 1;
        }
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, Bytes bytes)
    {
        Bits bitr = bytes.toBits();
        for (int i = 0; i < bitr.size(); i++)
        {
            if (cursor.get() < bits.size())
                bits.setBit(cursor.get(), bitr.getBit(i));
            cursor.advance();
        }
    }

    private void writeByte(Bits bits, Bits.Cursor cursor, int b)
    {
        writeBits(bits, cursor, encodeDataGcr(b >> 4), 5);
        writeBits(bits, cursor, encodeDataGcr(b), 5);
    }

    private void writeBytes(Bits bits, Bits.Cursor cursor, Bytes bytes)
    {
        for (int i = 0; i < bytes.size(); i++)
            writeByte(bits, cursor, bytes.getByte(i) & 0xff);
    }

    private void writeGap(Bits bits, Bits.Cursor cursor, int length)
    {
        for (int i = 0; i < length / 10; i++)
            writeByte(bits, cursor, '0');
    }

    private void writeSector(Bits bits,
                             Bits.Cursor cursor,
                             Victor9kEncoderProto.TrackdataProto trackdata,
                             Sector sector)
    {
        writeOneBits(bits, cursor, trackdata.getPreHeaderSyncBits());
        writeBits(bits, cursor, Victor9k.VICTOR9K_SECTOR_RECORD, 10);

        int encodedTrack = sector.location.logicalCylinder() | (sector.location.logicalHead() << 7);
        int encodedSector = sector.location.logicalSector();
        writeBytes(
                bits,
                cursor,
                Bytes.of(encodedTrack, encodedSector, (encodedTrack + encodedSector) & 0xff));

        writeGap(bits, cursor, trackdata.getPostHeaderGapBits());

        writeOneBits(bits, cursor, trackdata.getPreDataSyncBits());
        writeBits(bits, cursor, Victor9k.VICTOR9K_DATA_RECORD, 10);

        writeBytes(bits, cursor, sector.data);

        Bytes checksum = new Bytes(2);
        checksum.writer().writeLe16(Crc.sumBytes(sector.data));
        writeBytes(bits, cursor, checksum);
        writeGap(bits, cursor, trackdata.getPostDataGapBits());
    }

    private Victor9kEncoderProto.TrackdataProto getTrackFormat(int track, int head)
    {
        Victor9kEncoderProto.TrackdataProto.Builder builder =
                Victor9kEncoderProto.TrackdataProto.newBuilder();
        for (Victor9kEncoderProto.TrackdataProto f : config.getTrackdataList())
        {
            if (f.hasMinTrack() && (track < f.getMinTrack()))
                continue;
            if (f.hasMaxTrack() && (track > f.getMaxTrack()))
                continue;
            if (f.hasHead() && (head != f.getHead()))
                continue;

            builder.mergeFrom(f);
        }
        return builder.build();
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        Victor9kEncoderProto.TrackdataProto trackdata =
                getTrackFormat(ltl.logicalCylinder, ltl.logicalHead);

        int bitsPerRevolution =
                (int) ((trackdata.getRotationalPeriodMs() * 1e3) / trackdata.getClockPeriodUs());
        Bits bits = new Bits(bitsPerRevolution);
        long clockPeriod = (long) calculatePhysicalClockPeriodNs(
                trackdata.getClockPeriodUs() * 1e3,
                trackdata.getRotationalPeriodMs() * 1e6);
        Bits.Cursor cursor = new Bits.Cursor(0);

        bits.fillBitmapTo(
                cursor,
                (int) (trackdata.getPostIndexGapUs() * 1e3 / clockPeriod),
                new boolean[]{true, false});
        lastBit[0] = false;

        for (Sector sector : sectors)
            writeSector(bits, cursor, trackdata, sector);

        if (cursor.get() >= bits.size())
            throw new FluxEngineException(
                    "track data overrun by " + (cursor.get() - bits.size()) + " bits");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(bits, clockPeriod);
        return fluxmap;
    }
}
