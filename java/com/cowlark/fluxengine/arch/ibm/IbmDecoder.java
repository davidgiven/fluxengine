package com.cowlark.fluxengine.arch.ibm;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;
import com.cowlark.fluxengine.ibm.IbmDecoderProto;

/**
 * The IBM decoder, ported from arch/ibm/decoder.cc.
 */
public class IbmDecoder extends Decoder
{
    /*
     * The markers at the beginning of records are special, and have
     * missing clock pulses, allowing them to be found by the logic.
     *
     * IAM record:
     * flux:   XXXX-XXX-XXXX-X- = 0xf77a
     * clock:  X X - X - X X X  = 0xd7
     * data:    X X X X X X - - = 0xfc
     *
     * (We just ignore this one --- it's useless and optional.)
     */

    /*
     * IDAM record:
     * flux:   XXXX-X-X-XXXXXX- = 0xf57e
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X X X - = 0xfe
     */
    private static final FluxPattern FM_IDAM_PATTERN = new FluxPattern(16, 0xf57e);

    /*
     * DAM1 record:
     * flux:   XXXX-X-X-XX-X-X- = 0xf56a
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - - - = 0xf8
     */
    private static final FluxPattern FM_DAM1_PATTERN = new FluxPattern(16, 0xf56a);

    /*
     * DAM2 record:
     * flux:   XXXX-X-X-XX-XXXX = 0xf56f
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - X X = 0xfb
     */
    private static final FluxPattern FM_DAM2_PATTERN = new FluxPattern(16, 0xf56f);

    /*
     * TRS80DAM1 record:
     * flux:   XXXX-X-X-XX-X-XX = 0xf56b
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - - X = 0xf9
     */
    private static final FluxPattern FM_TRS80DAM1_PATTERN = new FluxPattern(16, 0xf56b);

    /*
     * TRS80DAM2 record:
     * flux:   XXXX-X-X-XX-XXX- = 0xf56e
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - X - = 0xfa
     */
    private static final FluxPattern FM_TRS80DAM2_PATTERN = new FluxPattern(16, 0xf56e);

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
    private static final FluxPattern MFM_PATTERN = new FluxPattern(48, 0x448944894489L);

    private static final FluxMatchers ANY_RECORD_PATTERN = FluxMatchers.of(
            MFM_PATTERN,
            FM_IDAM_PATTERN,
            FM_DAM1_PATTERN,
            FM_DAM2_PATTERN,
            FM_TRS80DAM1_PATTERN,
            FM_TRS80DAM2_PATTERN);

    private final IbmDecoderProto config;
    private int currentSectorSize;

    public IbmDecoder(DecoderProto config)
    {
        super(config);
        this.config = config.getIbm();
    }

    private IbmDecoderProto.TrackdataProto getTrackFormat(int track, int head)
    {
        IbmDecoderProto.TrackdataProto.Builder builder =
                IbmDecoderProto.TrackdataProto.newBuilder();
        for (IbmDecoderProto.TrackdataProto f : config.getTrackdataList())
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
    protected double advanceToNextRecord()
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        /* This is really annoying because the IBM record scheme has a
         * variable-sized header _and_ the checksum covers this header too. So
         * we have to read and decode a byte at a time until we know where the
         * record itself starts, saving the bytes for the checksumming later.
         */

        Bytes bytes = new Bytes();
        ByteWriter bw = bytes.writer();

        int id = readByte(bw);
        if (id == 0xa1)
        {
            readByte(bw);
            readByte(bw);
            id = readByte(bw);
        }
        if (id != Ibm.IBM_IDAM)
            return;

        ByteReader br = bytes.iterator();
        br.seek(bw.pos());

        Bits bits = readRawBits(Ibm.IBM_IDAM_LEN * 16);
        bw.write(FmMfm.decodeFmMfm(bits).slice(0, Ibm.IBM_IDAM_LEN));

        IbmDecoderProto.TrackdataProto trackdata =
                getTrackFormat(ltl.logicalCylinder, ltl.logicalHead);

        int logicalCylinder = br.read8();
        int logicalHead = br.read8();
        int logicalSector = br.read8();
        currentSectorSize = 1 << (br.read8() + 7);
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);

        int gotCrc = Crc.crc16(Crc.CCITT_POLY, bytes.slice(0, br.pos()));
        int wantCrc = br.readBe16();
        if (wantCrc == gotCrc)
            sector.status = Sector.Status.DATA_MISSING;

        if (trackdata.getIgnoreSideByte())
            sector.logicalLocation = new CylinderHeadSector(
                    sector.logicalLocation.cylinder(),
                    ltl.logicalHead,
                    sector.logicalLocation.sector());
        sector.logicalLocation = new CylinderHeadSector(
                sector.logicalLocation.cylinder(),
                sector.logicalLocation.head() ^ (trackdata.getInvertSideByte() ? 1 : 0),
                sector.logicalLocation.sector());
        if (trackdata.getIgnoreTrackByte())
            sector.logicalLocation = new CylinderHeadSector(
                    ltl.logicalCylinder,
                    sector.logicalLocation.head(),
                    sector.logicalLocation.sector());

        for (int s : trackdata.getIgnoreSectorList())
            if (sector.logicalLocation.sector() == s)
            {
                sector.status = Sector.Status.MISSING;
                break;
            }
    }

    @Override
    protected void decodeDataRecord()
    {
        /* This is the same deal as the sector record. */

        Bytes bytes = new Bytes();
        ByteWriter bw = bytes.writer();

        int id = readByte(bw);
        if (id == 0xa1)
        {
            readByte(bw);
            readByte(bw);
            id = readByte(bw);
        }
        if ((id != Ibm.IBM_DAM1) && (id != Ibm.IBM_DAM2) && (id != Ibm.IBM_TRS80DAM1) &&
                (id != Ibm.IBM_TRS80DAM2))
            return;

        ByteReader br = bytes.iterator();
        br.seek(bw.pos());

        Bits bits = readRawBits((currentSectorSize + 2) * 16);
        bw.write(FmMfm.decodeFmMfm(bits).slice(0, currentSectorSize + 2));

        sector.data = br.read(currentSectorSize);
        int gotCrc = Crc.crc16(Crc.CCITT_POLY, bytes.slice(0, br.pos()));
        int wantCrc = br.readBe16();
        sector.status = (wantCrc == gotCrc) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;

        if (currentSectorSize != ltl.sectorSize)
            System.err.printf(
                    "Warning: configured sector size for t%d.h%d.s%d is %d bytes but that seen on" +
                            " disk is %d bytes%n",
                    sector.logicalLocation.cylinder(),
                    sector.logicalLocation.head(),
                    sector.logicalLocation.sector(),
                    ltl.sectorSize,
                    currentSectorSize);
    }

    private int readByte(ByteWriter bw)
    {
        Bits bits = readRawBits(16);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, 1);
        int byte0 = bytes.getByte(0) & 0xff;
        bw.write8(byte0);
        return byte0;
    }
}