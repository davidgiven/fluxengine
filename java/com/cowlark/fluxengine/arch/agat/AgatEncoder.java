package com.cowlark.fluxengine.arch.agat;

import com.cowlark.fluxengine.agat.AgatEncoderProto;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.FmMfm;
import java.util.List;

/**
 * The Agat encoder, ported from arch/agat/encoder.cc.
 */
public class AgatEncoder extends Encoder
{
    private final AgatEncoderProto config;
    private final boolean[] lastBit = new boolean[1];
    private Bits bits;
    private Bits.Cursor cursor;

    public AgatEncoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getAgat();
    }

    private void writeRawBits(long data, int width)
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

    private void writeBytes(Bytes bytes)
    {
        FmMfm.encodeMfm(bits, cursor, bytes, lastBit);
    }

    private void writeByte(int byte_)
    {
        Bytes b = new Bytes(1);
        b.writer().write8(byte_);
        writeBytes(b);
    }

    private void writeFillerRawBytes(int count, int byte_)
    {
        for (int i = 0; i < count; i++)
            writeRawBits(byte_, 16);
    }

    private void writeFillerBytes(int count, int byte_)
    {
        Bytes b = new Bytes(1);
        b.writer().write8(byte_);
        for (int i = 0; i < count; i++)
            writeBytes(b);
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        double clockRateUs = config.getTargetClockPeriodUs() / 2.0;
        int bitsPerRevolution =
                (int) ((config.getTargetRotationalPeriodMs() * 1000.0) / clockRateUs);
        bits = new Bits(bitsPerRevolution);
        cursor = new Bits.Cursor(0);

        writeFillerRawBytes(config.getPostIndexGapBytes(), 0xaaaa);

        for (Sector sector : sectors)
        {
            /* Header */

            writeFillerRawBytes(config.getPreSectorGapBytes(), 0xaaaa);
            writeRawBits(Agat.SECTOR_ID, 64);
            writeByte(0x5a);
            writeByte((sector.logicalLocation.cylinder() << 1) | sector.logicalLocation.head());
            writeByte(sector.logicalLocation.sector());
            writeByte(0x5a);

            /* Data */

            writeFillerRawBytes(config.getPreDataGapBytes(), 0xaaaa);
            Bytes data = sector.data.slice(0, Agat.AGAT_SECTOR_SIZE);
            writeRawBits(Agat.DATA_ID, 64);
            writeBytes(data);
            writeByte(Agat.agatChecksum(data));
            writeByte(0x5a);
        }

        if (cursor.get() >= bits.size())
            throw new FluxEngineException("track data overrun");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits, (long) calculatePhysicalClockPeriodNs(
                        config.getTargetClockPeriodUs() * 1e3,
                        config.getTargetRotationalPeriodMs() * 1e6));
        return fluxmap;
    }
}
