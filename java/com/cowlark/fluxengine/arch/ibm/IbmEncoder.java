package com.cowlark.fluxengine.arch.ibm;

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
import com.cowlark.fluxengine.ibm.IbmEncoderProto;
import java.util.List;

/**
 * The IBM encoder, ported from arch/ibm/encoder.cc.
 */
public class IbmEncoder extends Encoder
{
    /* IAM record separator:
     * 0xC2 is:
     * data:    1  1  0  0  0  0  1  0  = 0xc2
     * mfm:     01 01 00 10 10 10 01 00 = 0x5254
     * special: 01 01 00 10 00 10 01 00 = 0x5224
     */
    private static final int MFM_IAM_SEPARATOR = 0x5224;

    /* FM IAM record:
     * flux:   XXXX-XXX-XXXX-X- = 0xf77a
     * clock:  X X - X - X X X  = 0xd7
     * data:    X X X X X X - - = 0xfc
     */
    private static final int FM_IAM_RECORD = 0xf77a;

    /* MFM IAM record:
     * data:   1  1  1  1  1  1  0  0  = 0xfc
     * flux:   01 01 01 01 01 01 00 10 = 0x5552
     */
    private static final int MFM_IAM_RECORD = 0x5552;

    /* MFM record separator:
     * 0xA1 is:
     * data:    1  0  1  0  0  0  0  1  = 0xa1
     * mfm:     01 00 01 00 10 10 10 01 = 0x44a9
     * special: 01 00 01 00 10 00 10 01 = 0x4489
     *                       ^^^^^
     * When shifted out of phase, the special 0xa1 byte becomes an illegal
     * encoding (you can't do 10 00). So this can't be spoofed by user data.
     *
     * shifted: 10 00 10 01 00 01 00 1
     *
     * It's repeated three times.
     */
    private static final int MFM_RECORD_SEPARATOR = 0x4489;
    private static final int MFM_RECORD_SEPARATOR_BYTE = 0xa1;
    private final ConfigProto fullConfig;
    private final IbmEncoderProto config;
    private final boolean[] lastBit = new boolean[1];
    private Bits bits;
    private Bits.Cursor cursor;

    public IbmEncoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getIbm();
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

    private void writeBytes(Bytes bytes, IbmEncoderProto.TrackdataProto trackdata)
    {
        if (trackdata.getUseFm())
            FmMfm.encodeFm(bits, cursor, bytes);
        else
            FmMfm.encodeMfm(bits, cursor, bytes, lastBit);
    }

    private void writeFillerRawBytes(int count, int byte_)
    {
        for (int i = 0; i < count; i++)
            writeRawBits(byte_, 16);
    }

    private void writeFillerBytes(int count, int byte_, IbmEncoderProto.TrackdataProto trackdata)
    {
        Bytes b = Bytes.of(byte_);
        for (int i = 0; i < count; i++)
            writeBytes(b, trackdata);
    }

    private IbmEncoderProto.TrackdataProto getEncoderTrackData(int track, int head)
    {
        IbmEncoderProto.TrackdataProto.Builder builder =
                IbmEncoderProto.TrackdataProto.newBuilder();
        for (IbmEncoderProto.TrackdataProto f : config.getTrackdataList())
        {
            if (f.hasTrack() && (f.getTrack() != track))
                continue;
            if (f.hasHead() && (f.getHead() != head))
                continue;

            builder.mergeFrom(f);
        }
        return builder.build();
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        IbmEncoderProto.TrackdataProto trackdata =
                getEncoderTrackData(ltl.logicalCylinder, ltl.logicalHead);

        double clockRateUs = trackdata.getTargetClockPeriodUs();
        if (!trackdata.getUseFm())
            clockRateUs /= 2.0;
        int bitsPerRevolution =
                (int) ((trackdata.getTargetRotationalPeriodMs() * 1000.0) / clockRateUs);
        bits = new Bits(bitsPerRevolution);
        cursor = new Bits.Cursor(0);

        int idamUnencoded = decodeUint16(trackdata.getIdamByte());
        int damUnencoded = decodeUint16(trackdata.getDamByte());

        int sectorSize = 0;
        {
            int s = ltl.sectorSize >> 7;
            while (s > 1)
            {
                s >>= 1;
                sectorSize += 1;
            }
        }

        int gapFill = trackdata.getGapFillByte();

        writeFillerRawBytes(trackdata.getGap0(), gapFill);
        if (trackdata.getEmitIam())
        {
            writeFillerBytes(trackdata.getUseFm() ? 6 : 12, 0x00, trackdata);
            if (!trackdata.getUseFm())
            {
                for (int i = 0; i < 3; i++)
                    writeRawBits(MFM_IAM_SEPARATOR, 16);
            }
            writeRawBits(trackdata.getUseFm() ? FM_IAM_RECORD : MFM_IAM_RECORD, 16);
            writeFillerRawBytes(trackdata.getGap1(), gapFill);
        }

        boolean first = true;
        for (Sector sectorData : sectors)
        {
            if (!first)
                writeFillerRawBytes(trackdata.getGap3(), gapFill);
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

                writeFillerBytes(trackdata.getUseFm() ? 6 : 12, 0x00, trackdata);
                if (!trackdata.getUseFm())
                {
                    for (int i = 0; i < 3; i++)
                        bw.write8(MFM_RECORD_SEPARATOR_BYTE);
                }
                bw.write8(idamUnencoded);
                bw.write8(sectorData.location.logicalCylinder());
                bw.write8(sectorData.location.logicalHead() ^
                        (trackdata.getInvertSideByte() ? 1 : 0));
                bw.write8(sectorData.location.logicalSector());
                bw.write8(sectorSize);
                int crc = Crc.crc16(Crc.CCITT_POLY, header);
                bw.writeBe16(crc);

                int conventionalHeaderStart = 0;
                if (!trackdata.getUseFm())
                {
                    for (int i = 0; i < 3; i++)
                        writeRawBits(MFM_RECORD_SEPARATOR, 16);
                    conventionalHeaderStart += 3;
                }
                writeRawBits(trackdata.getIdamByte(), 16);
                conventionalHeaderStart += 1;

                writeBytes(header.slice(conventionalHeaderStart), trackdata);
            }

            writeFillerRawBytes(trackdata.getGap2(), gapFill);

            {
                Bytes data = new Bytes(0);
                ByteWriter bw = data.writer();

                writeFillerBytes(trackdata.getUseFm() ? 6 : 12, 0x00, trackdata);
                if (!trackdata.getUseFm())
                {
                    for (int i = 0; i < 3; i++)
                        bw.write8(MFM_RECORD_SEPARATOR_BYTE);
                }
                bw.write8(damUnencoded);

                Bytes truncatedData = sectorData.data.slice(0, ltl.sectorSize);
                bw.write(truncatedData);
                int crc = Crc.crc16(Crc.CCITT_POLY, data);
                bw.writeBe16(crc);

                int conventionalHeaderStart = 0;
                if (!trackdata.getUseFm())
                {
                    for (int i = 0; i < 3; i++)
                        writeRawBits(MFM_RECORD_SEPARATOR, 16);
                    conventionalHeaderStart += 3;
                }
                writeRawBits(trackdata.getDamByte(), 16);
                conventionalHeaderStart += 1;

                writeBytes(data.slice(conventionalHeaderStart), trackdata);
            }
        }

        if (cursor.get() >= bits.size())
            throw new FluxEngineException("track data overrun");
        while (cursor.get() < bits.size())
            writeFillerRawBytes(1, gapFill);

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits, (long) calculatePhysicalClockPeriod(
                        fullConfig,
                        clockRateUs * 1e3,
                        trackdata.getTargetRotationalPeriodMs() * 1e6));
        return fluxmap;
    }
}
