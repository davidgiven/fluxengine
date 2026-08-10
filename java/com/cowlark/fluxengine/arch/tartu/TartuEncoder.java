package com.cowlark.fluxengine.arch.tartu;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;
import com.cowlark.fluxengine.tartu.TartuEncoderProto;
import java.util.List;

/**
 * The Tartu encoder, ported from arch/tartu/encoder.cc.
 */
public class TartuEncoder extends Encoder
{
    private final TartuEncoderProto config;
    private final boolean[] lastBit = new boolean[1];
    private double clockRateUs;
    private Bits bits;
    private Bits.Cursor cursor;

    public TartuEncoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getTartu();
    }

    private void writeBytes(Bytes bytes)
    {
        FmMfm.encodeMfm(bits, cursor, bytes, lastBit);
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

    private void writeFillerRawBitsUs(double us)
    {
        int count = (int) ((us / clockRateUs) / 2);
        for (int i = 0; i < count; i++)
            writeRawBits(0b10, 2);
    }

    private void writeSector(Sector sectorData)
    {
        writeRawBits(config.getHeaderMarker(), 64);
        {
            Bytes bytes = new Bytes(0);
            ByteWriter bw = bytes.writer();
            bw.write8((sectorData.location.logicalCylinder() << 1) |
                    sectorData.location.logicalHead());
            bw.write8(1);
            bw.write8(sectorData.location.logicalSector());
            bw.write8(~Crc.sumBytes(bytes.slice(0, 3)));
            writeBytes(bytes);
        }

        writeFillerRawBitsUs(config.getGap3Us());
        writeRawBits(config.getDataMarker(), 64);
        {
            Bytes bytes = new Bytes(0);
            ByteWriter bw = bytes.writer();
            bw.write(sectorData.data);
            bw.write8(~Crc.sumBytes(bytes.slice(0, sectorData.data.size())));
            writeBytes(bytes);
        }
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        clockRateUs = config.getClockPeriodUs();
        int bitsPerRevolution =
                (int) ((config.getTargetRotationalPeriodMs() * 1000.0) / clockRateUs);

        bits = new Bits(bitsPerRevolution);
        cursor = new Bits.Cursor(0);

        writeFillerRawBitsUs(config.getGap1Us());
        boolean first = true;
        for (Sector sectorData : sectors)
        {
            if (!first)
                writeFillerRawBitsUs(config.getGap4Us());
            first = false;
            writeSector(sectorData);
        }

        if (cursor.get() > bits.size())
            throw new FluxEngineException("track data overrun");
        writeFillerRawBitsUs(config.getTargetRotationalPeriodMs() * 1000.0);

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits,
                (long) calculatePhysicalClockPeriodNs(
                        clockRateUs * 1e3,
                        config.getTargetRotationalPeriodMs() * 1e6));
        return fluxmap;
    }
}
