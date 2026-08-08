package com.cowlark.fluxengine.arch.tartu;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The Tartu decoder, ported from arch/tartu/decoder.cc.
 */
public class TartuDecoder extends Decoder
{
    private static final FluxPattern HEADER_PATTERN = new FluxPattern(64, Tartu.HEADER_BITS);
    private static final FluxPattern DATA_PATTERN = new FluxPattern(64, Tartu.DATA_BITS);

    private static final FluxMatchers ANY_RECORD_PATTERN = FluxMatchers.of(HEADER_PATTERN, DATA_PATTERN);

    public TartuDecoder(DecoderProto config)
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
        if (readRaw64() != Tartu.HEADER_BITS)
            return;

        Bits bits = readRawBits(16 * 4);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, 4);

        ByteReader br = bytes.iterator();
        int track = br.read8();
        int logicalCylinder = track >> 1;
        int logicalHead = track & 1;
        br.skip(1); /* seems always to be 1 */
        int logicalSector = br.read8();
        int wantChecksum = br.read8();
        int gotChecksum = ~Crc.sumBytes(bytes.slice(0, 3)) & 0xff;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);

        if (wantChecksum == gotChecksum)
            sector.status = Sector.Status.DATA_MISSING;

        sector.status = Sector.Status.DATA_MISSING;
    }

    @Override
    protected void decodeDataRecord()
    {
        if (readRaw64() != Tartu.DATA_BITS)
            return;

        Bits bits = readRawBits(129 * 16);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, 129);
        sector.data = bytes.slice(0, 128);

        int wantChecksum = bytes.iterator().seek(128).read8();
        int gotChecksum = ~Crc.sumBytes(sector.data) & 0xff;
        sector.status = (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}