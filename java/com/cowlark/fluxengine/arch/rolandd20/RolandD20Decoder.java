package com.cowlark.fluxengine.arch.rolandd20;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The Roland D20 decoder, ported from arch/rolandd20/decoder.cc.
 */
public class RolandD20Decoder extends Decoder
{
    /* Sector header record:
     *
     * BF FF FF FF FF FF FE AB
     *
     * This encodes to:
     *
     *    e    d    5    5    5    5    5    5
     * 1110 1101 0101 0101 0101 0101 0101 0101
     *    5    5    5    5    5    5    5    5
     * 0101 0101 0101 0101 0101 0101 0101 0101
     *    5    5    5    5    5    5    5    5
     * 0101 0101 0101 0101 0101 0101 0101 0101
     *    5    5    5    4    4    4    4    5
     * 0101 0101 0101 0100 0100 0100 0100 0101
     */
    private static final FluxPattern SECTOR_PATTERN = new FluxPattern(64, 0xed55555555555555L);

    public RolandD20Decoder(DecoderProto config)
    {
        super(config);
    }

    private static void hexdump(Bytes buffer)
    {
        int pos = 0;

        while (pos < buffer.size())
        {
            System.out.printf("%05x : ", pos);
            for (int i = 0; i < 16; i++)
            {
                if ((pos + i) < buffer.size())
                    System.out.printf("%02x ", buffer.getByte(pos + i));
                else
                    System.out.print("-- ");
            }
            System.out.print(" : ");
            for (int i = 0; i < 16; i++)
            {
                if ((pos + i) >= buffer.size())
                    break;

                int c = buffer.getByte(pos + i) & 0xff;
                if ((c >= 32) && (c <= 126))
                    System.out.print((char) c);
                else
                    System.out.print('.');
            }
            System.out.println();

            pos += 16;
        }
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(SECTOR_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        Bytes bytes = FmMfm.decodeFmMfm(readRawBits(256));
        System.out.printf("%.3f ", sector.clockNs);
        hexdump(bytes);
    }
}