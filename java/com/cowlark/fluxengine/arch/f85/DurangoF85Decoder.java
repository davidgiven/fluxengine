package com.cowlark.fluxengine.arch.f85;

import com.cowlark.fluxengine.core.BitWriter;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
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
 * The Durango F85 decoder, ported from arch/f85/decoder.cc.
 */
public class DurangoF85Decoder extends Decoder
{
    private static final FluxPattern SECTOR_RECORD_PATTERN = new FluxPattern(24, F85.F85_SECTOR_RECORD);
    private static final FluxPattern DATA_RECORD_PATTERN = new FluxPattern(24, F85.F85_DATA_RECORD);
    private static final FluxMatchers ANY_RECORD_PATTERN =
            FluxMatchers.of(SECTOR_RECORD_PATTERN, DATA_RECORD_PATTERN);

    private static int decodeDataGcr(int gcr)
    {
        switch (gcr)
        {
            case 0x19: return 0x00;
            case 0x1b: return 0x01;
            case 0x12: return 0x02;
            case 0x13: return 0x03;
            case 0x1d: return 0x04;
            case 0x15: return 0x05;
            case 0x16: return 0x06;
            case 0x17: return 0x07;
            case 0x1a: return 0x08;
            case 0x09: return 0x09;
            case 0x0a: return 0x0a;
            case 0x0b: return 0x0b;
            case 0x1e: return 0x0c;
            case 0x0d: return 0x0d;
            case 0x0e: return 0x0e;
            case 0x0f: return 0x0f;
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

    public DurangoF85Decoder(DecoderProto config)
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
        /* Skip sync bits and ID byte. */

        if (readRaw24() != F85.F85_SECTOR_RECORD)
            return;

        /* Read header. */

        Bytes bytes = decode(readRawBits(6 * 10));

        int logicalSector = bytes.getByte(2) & 0xff;
        int logicalHead = 0;
        int logicalCylinder = bytes.getByte(0) & 0xff;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);

        int wantChecksum = bytes.iterator().seek(4).readBe16();
        int gotChecksum = Crc.crc16(Crc.CCITT_POLY, 0xef21, bytes.slice(0, 4));
        if (wantChecksum == gotChecksum)
            sector.status = Sector.Status.DATA_MISSING; /* unintuitive but correct */
    }

    @Override
    protected void decodeDataRecord()
    {
        /* Skip sync bits ID byte. */

        if (readRaw24() != F85.F85_DATA_RECORD)
            return;

        Bytes bytes = decode(readRawBits((F85.F85_SECTOR_LENGTH + 3) * 10))
                .slice(0, F85.F85_SECTOR_LENGTH + 3);
        ByteReader br = bytes.iterator();

        sector.data = br.read(F85.F85_SECTOR_LENGTH);
        int wantChecksum = br.readBe16();
        int gotChecksum = Crc.crc16(Crc.CCITT_POLY, 0xbf84, sector.data);
        sector.status = (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}