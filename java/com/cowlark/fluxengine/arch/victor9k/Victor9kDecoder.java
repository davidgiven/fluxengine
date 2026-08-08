package com.cowlark.fluxengine.arch.victor9k;

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
 * The Victor 9k decoder, ported from arch/victor9k/decoder.cc.
 */
public class Victor9kDecoder extends Decoder
{
    private static final FluxPattern SECTOR_RECORD_PATTERN = new FluxPattern(32, Victor9k.VICTOR9K_SECTOR_RECORD);
    private static final FluxPattern DATA_RECORD_PATTERN = new FluxPattern(32, Victor9k.VICTOR9K_DATA_RECORD);
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

            int decoded = decodeDataGcr(inputfifo);
            bitw.push(decoded, 4);
        }
        bitw.flush();

        return output;
    }

    public Victor9kDecoder(DecoderProto config)
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
        /* Check the ID. */

        if (readRaw32() != Victor9k.VICTOR9K_SECTOR_RECORD)
            return;

        /* Read header. */

        Bytes bytes = decode(readRawBits(3 * 10)).slice(0, 3);

        int rawTrack = bytes.getByte(0) & 0xff;
        int logicalSector = bytes.getByte(1) & 0xff;
        int gotChecksum = bytes.getByte(2) & 0xff;

        int logicalCylinder = rawTrack & 0x7f;
        int logicalHead = rawTrack >> 7;
        int wantChecksum = (bytes.getByte(0) & 0xff) + (bytes.getByte(1) & 0xff);
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);
        if ((logicalSector > 20) || (logicalCylinder > 85) || (logicalHead > 1))
            return;

        if (wantChecksum == gotChecksum)
            sector.status = Sector.Status.DATA_MISSING; /* unintuitive but correct */
    }

    @Override
    protected void decodeDataRecord()
    {
        /* Check the ID. */

        if (readRaw32() != Victor9k.VICTOR9K_DATA_RECORD)
            return;

        /* Read data. */

        Bytes bytes = decode(readRawBits((Victor9k.VICTOR9K_SECTOR_LENGTH + 4) * 10))
                .slice(0, Victor9k.VICTOR9K_SECTOR_LENGTH + 4);
        ByteReader br = bytes.iterator();

        sector.data = br.read(Victor9k.VICTOR9K_SECTOR_LENGTH);
        int gotChecksum = Crc.sumBytes(sector.data);
        int wantChecksum = br.readLe16();
        sector.status = (gotChecksum == wantChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}