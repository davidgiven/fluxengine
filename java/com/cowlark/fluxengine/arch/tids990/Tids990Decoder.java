package com.cowlark.fluxengine.arch.tids990;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The Texas Instruments DS990 decoder, ported from arch/tids990/decoder.cc.
 */
public class Tids990Decoder extends Decoder
{
    /*
     * The Texas Instruments DS990 uses MFM with a scheme similar to a
     * simplified version of the IBM record scheme (it's actually easier to
     * parse than IBM). There are 26 sectors per track, each holding a rather
     * weird 288 bytes.
     */

    /*
     * Sector record:
     * data:    0  1  0  1  0  1  0  1 .0  0  0  0  1  0  1  0  = 0x550a
     * mfm:     00 01 00 01 00 01 00 01.00 10 10 10 01 00 01 00 = 0x11112a44
     * special: 00 01 00 01 00 01 00 01.00 10 00 10 01 00 01 00 = 0x11112244
     *                                        ^^
     * When shifted out of phase, the special 0xa1 byte becomes an illegal
     * encoding (you can't do 10 00). So this can't be spoofed by user data.
     */
    private static final int SECTOR_ID = 0x550a;
    private static final FluxPattern SECTOR_RECORD_PATTERN = new FluxPattern(32, 0x11112244);

    /*
     * Data record:
     * data:    0  1  0  1  0  1  0  1 .0  0  0  0  1  0  1  1  = 0x550b
     * mfm:     00 01 00 01 00 01 00 01.00 10 10 10 01 00 01 01 = 0x11112a45
     * special: 00 01 00 01 00 01 00 01.00 10 00 10 01 00 01 01 = 0x11112245
     *                                        ^^
     * When shifted out of phase, the special 0xa1 byte becomes an illegal
     * encoding (you can't do 10 00). So this can't be spoofed by user data.
     */
    private static final int DATA_ID = 0x550b;
    private static final FluxPattern DATA_RECORD_PATTERN = new FluxPattern(32, 0x11112245);
    private static final FluxMatchers ANY_RECORD_PATTERN =
            FluxMatchers.of(SECTOR_RECORD_PATTERN, DATA_RECORD_PATTERN);

    public Tids990Decoder(DecoderProto config)
    {
        super(config);
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        Bits bits = readRawBits(Tids990.TIDS990_SECTOR_RECORD_SIZE * 16);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, Tids990.TIDS990_SECTOR_RECORD_SIZE);

        ByteReader br = bytes.iterator();
        if (br.readBe16() != SECTOR_ID)
            return;

        int gotChecksum =
                Crc.crc16(Crc.CCITT_POLY, bytes.slice(1, Tids990.TIDS990_SECTOR_RECORD_SIZE - 3));

        int logicalHead = br.read8() >> 3;
        int logicalCylinder = br.read8();
        br.read8(); /* number of sectors per track */
        int logicalSector = br.read8();
        br.readBe16(); /* sector size */
        int wantChecksum = br.readBe16();
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);

        if (wantChecksum == gotChecksum)
            sector.status = Sector.Status.DATA_MISSING;
    }

    @Override
    protected void decodeDataRecord()
    {
        Bits bits = readRawBits(Tids990.TIDS990_DATA_RECORD_SIZE * 16);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, Tids990.TIDS990_DATA_RECORD_SIZE);

        ByteReader br = bytes.iterator();
        if (br.readBe16() != DATA_ID)
            return;

        int gotChecksum =
                Crc.crc16(Crc.CCITT_POLY, bytes.slice(1, Tids990.TIDS990_DATA_RECORD_SIZE - 3));

        sector.data = br.read(Tids990.TIDS990_PAYLOAD_SIZE);
        int wantChecksum = br.readBe16();
        sector.status =
                (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}