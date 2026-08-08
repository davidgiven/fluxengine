package com.cowlark.fluxengine.arch.c64;

import com.cowlark.fluxengine.core.BitWriter;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;

/**
 * The Commodore 64 decoder, ported from arch/c64/decoder.cc.
 */
public class Commodore64Decoder extends Decoder
{
    private static final FluxPattern SECTOR_RECORD_PATTERN = new FluxPattern(20, C64.C64_SECTOR_RECORD);
    private static final FluxPattern DATA_RECORD_PATTERN = new FluxPattern(20, C64.C64_DATA_RECORD);
    private static final FluxMatchers ANY_RECORD_PATTERN =
            FluxMatchers.of(SECTOR_RECORD_PATTERN, DATA_RECORD_PATTERN);

    private static int decodeDataGcr(int gcr)
    {
        switch (gcr)
        {
            case 0x0a: return 0x0;
            case 0x0b: return 0x1;
            case 0x12: return 0x2;
            case 0x13: return 0x3;
            case 0x0e: return 0x4;
            case 0x0f: return 0x5;
            case 0x16: return 0x6;
            case 0x17: return 0x7;
            case 0x09: return 0x8;
            case 0x19: return 0x9;
            case 0x1a: return 0xa;
            case 0x1b: return 0xb;
            case 0x0d: return 0xc;
            case 0x1d: return 0xd;
            case 0x1e: return 0xe;
            case 0x15: return 0xf;
            default: return -1;
        }
    }

    private static Bytes decode(Bits bits)
    {
        Bytes output = new Bytes();
        ByteWriter bw = new ByteWriter(output);
        BitWriter bitw = new BitWriter(bw);

        int ii = 0;
        while (ii < bits.size())
        {
            int inputfifo = 0;
            for (int i = 0; i < 5; i++)
            {
                if (ii >= bits.size())
                    break;
                inputfifo = (inputfifo << 1) | (bits.getBit(ii++) ? 1 : 0);
            }

            bitw.push(decodeDataGcr(inputfifo), 4);
        }
        bitw.flush();

        return output;
    }

    public Commodore64Decoder(DecoderProto config)
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
        if (readRaw20() != C64.C64_SECTOR_RECORD)
            return;

        Bits bits = readRawBits(5 * 10);
        Bytes bytes = decode(bits).slice(0, 5);

        int checksum = bytes.getByte(0) & 0xff;
        int logicalSector = bytes.getByte(1) & 0xff;
        int logicalHead = 0;
        int logicalCylinder = (bytes.getByte(2) & 0xff) - 1;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);
        if (checksum == Crc.xorBytes(bytes.slice(1, 4)))
            sector.status = Sector.Status.DATA_MISSING; /* unintuitive but correct */
    }

    @Override
    protected void decodeDataRecord()
    {
        if (readRaw20() != C64.C64_DATA_RECORD)
            return;

        Bits bits = readRawBits(259 * 10);
        Bytes bytes = decode(bits).slice(0, 259);

        sector.data = bytes.slice(0, C64.C64_SECTOR_LENGTH);
        int gotChecksum = Crc.xorBytes(sector.data);
        int wantChecksum = bytes.getByte(256) & 0xff;
        sector.status = (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}