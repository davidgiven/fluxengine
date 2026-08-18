package com.cowlark.fluxengine.arch.tids990;

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
import com.cowlark.fluxengine.tids990.Tids990EncoderProto;
import java.util.List;

/**
 * The TI DS990 encoder, ported from arch/tids990/encoder.cc.
 */
public class Tids990Encoder extends Encoder
{
    private final Tids990EncoderProto config;
    private final boolean[] lastBit = new boolean[1];
    private Bits bits;
    private Bits.Cursor cursor;

    public Tids990Encoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getTids990();
    }

    private static int decodeUint16(int raw)
    {
        Bytes b = new Bytes(2);
        b.writer().writeBe16(raw);
        return FmMfm.decodeFmMfm(b.toBits()).getByte(0) & 0xff;
    }

    private void writeRawBits(int data, int width)
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

    private void writeBytes(int count, int byte_)
    {
        Bytes bytes = Bytes.of(byte_);
        for (int i = 0; i < count; i++)
            writeBytes(bytes);
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        double clockRateUs = config.getClockPeriodUs() / 2.0;
        int bitsPerRevolution = (int) ((config.getRotationalPeriodMs() * 1000.0) / clockRateUs);
        bits = new Bits(bitsPerRevolution);
        cursor = new Bits.Cursor(0);

        int am1Unencoded = decodeUint16(config.getAm1Byte());
        int am2Unencoded = decodeUint16(config.getAm2Byte());

        writeBytes(config.getGap1Bytes(), 0x55);

        boolean first = true;
        for (Sector sectorData : sectors)
        {
            if (!first)
                writeBytes(config.getGap3Bytes(), 0x55);
            first = false;

            /* Writing the sector and data records are fantastically annoying.
             * The CRC is calculated from the *very start* of the record, and
             * include the malformed marker bytes. Our encoder doesn't know
             * about this, of course, with the result that we have to construct
             * the unencoded header, calculate the checksum, and then use the
             * same logic to emit the bytes which require special encoding
             * before encoding the rest of the header normally. */

            {
                Bytes header = new Bytes(0);
                ByteWriter bw = header.writer();

                writeBytes(12, 0x55);
                bw.write8(am1Unencoded);
                bw.write8(sectorData.location.logicalHead() << 3);
                bw.write8(sectorData.location.logicalCylinder());
                bw.write8(config.getSectorCount());
                bw.write8(sectorData.location.logicalSector());
                bw.writeBe16(sectorData.data.size());
                int crc = Crc.crc16(Crc.CCITT_POLY, header);
                bw.writeBe16(crc);

                writeRawBits(config.getAm1Byte(), 16);
                writeBytes(header.slice(1));
            }

            writeBytes(config.getGap2Bytes(), 0x55);

            {
                Bytes data = new Bytes(0);
                ByteWriter bw = data.writer();

                writeBytes(12, 0x55);
                bw.write8(am2Unencoded);

                bw.write(sectorData.data);
                int crc = Crc.crc16(Crc.CCITT_POLY, data);
                bw.writeBe16(crc);

                writeRawBits(config.getAm2Byte(), 16);
                writeBytes(data.slice(1));
            }
        }

        if (cursor.get() >= bits.size())
            throw new FluxEngineException("track data overrun");
        while (cursor.get() < bits.size())
            writeBytes(1, 0x55);

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(bits,
                (long) calculatePhysicalClockPeriodNs(clockRateUs * 1e3,
                        config.getRotationalPeriodMs() * 1e6));
        return fluxmap;
    }
}
