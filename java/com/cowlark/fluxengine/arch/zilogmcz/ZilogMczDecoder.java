package com.cowlark.fluxengine.arch.zilogmcz;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The Zilog MCZ decoder, ported from arch/zilogmcz/decoder.cc.
 */
public class ZilogMczDecoder extends Decoder
{
    private static final FluxPattern SECTOR_START_PATTERN = new FluxPattern(16, 0xaaab);

    public ZilogMczDecoder(DecoderProto config)
    {
        super(config);
    }

    @Override
    protected double advanceToNextRecord()
    {
        seekToIndexMark();
        return seekToPattern(SECTOR_START_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        readRawBits(14);

        Bits rawbits = readRawBits(140 * 16);
        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, 140);
        ByteReader br = bytes.iterator();

        int logicalSector = br.read8() & 0x1f;
        int logicalHead = 0;
        int logicalCylinder = br.read8() & 0x7f;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);
        if (logicalSector > 31)
            return;
        if (logicalCylinder > 80)
            return;

        sector.data = br.read(132);
        int wantChecksum = br.readBe16();
        int gotChecksum = Crc.crc16(Crc.MODBUS_POLY, 0x0000, bytes.slice(0, 134));

        sector.status = (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}