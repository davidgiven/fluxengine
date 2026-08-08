package com.cowlark.fluxengine.arch.amiga;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The Amiga decoder, ported from arch/amiga/decoder.cc.
 */
public class AmigaDecoder extends Decoder
{
    /*
     * Amiga disks use MFM but it's not quite the same as IBM MFM. They only
     * use a single type of record with a different marker byte.
     *
     * See the big comment in the IBM MFM decoder for the gruesome details of
     * how MFM works.
     */
    private static final FluxPattern SECTOR_PATTERN =
            new FluxPattern(48, Amiga.AMIGA_SECTOR_RECORD);

    public AmigaDecoder(DecoderProto config)
    {
        super(config);
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(SECTOR_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        if (readRaw48() != Amiga.AMIGA_SECTOR_RECORD)
            return;

        Bits rawbits = readRawBits(Amiga.AMIGA_RECORD_SIZE * 16);
        if (rawbits.size() < (Amiga.AMIGA_RECORD_SIZE * 16))
            return;
        Bytes rawbytes = rawbits.toBytes().slice(0, Amiga.AMIGA_RECORD_SIZE * 2);
        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, Amiga.AMIGA_RECORD_SIZE);

        int[] index = {0};

        Bytes header = Amiga.amigaDeinterleave(bytes, index, 4);
        Bytes recoveryinfo = Amiga.amigaDeinterleave(bytes, index, 16);

        int logicalCylinder = (header.getByte(1) & 0xff) >> 1;
        int logicalHead = header.getByte(1) & 1;
        int logicalSector = header.getByte(2) & 0xff;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);

        int wantedheaderchecksum = Amiga.amigaDeinterleave(bytes, index, 4).iterator().readBe32();
        int gotheaderchecksum = Amiga.amigaChecksum(rawbytes.slice(0, 40));
        if (gotheaderchecksum != wantedheaderchecksum)
            return;

        int wanteddatachecksum = Amiga.amigaDeinterleave(bytes, index, 4).iterator().readBe32();
        int gotdatachecksum = Amiga.amigaChecksum(rawbytes.slice(56, 1024));

        Bytes data = new Bytes();
        data.writer().write(Amiga.amigaDeinterleave(bytes, index, 512)).write(recoveryinfo);
        sector.data = data;
        sector.status = (gotdatachecksum == wanteddatachecksum) ?
                Sector.Status.OK :
                Sector.Status.BAD_CHECKSUM;
    }
}