package com.cowlark.fluxengine.external;

import com.cowlark.fluxengine.core.BitReader;
import com.cowlark.fluxengine.core.BitWriter;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;

/**
 * FM and MFM encode/decode helpers, ported from lib/decoders/fmmfm.cc.
 * The {@code cursor} and {@code lastBit} parameters of the encoding functions
 * are carried in single-element arrays to provide the in/out semantics of the
 * C++ references.
 */
public final class FmMfm
{
    private FmMfm()
    {
    }

    /*
     * FM is dumb as rocks, consisting on regular clock pulses with data pulses
     * in the gaps. 0x00 is:
     *
     *     X-X-X-X-X-X-X-X-
     *
     * 0xff is:
     *
     *     XXXXXXXXXXXXXXXX
     *
     * So we just need to extract all the odd bits.
     *
     * MFM and M2FM are slightly more complicated, where the first bit of each
     * pair can be either 0 or 1... but the second bit is always the data bit,
     * and at this point we simply don't care what the first bit is, so
     * decoding MFM uses just the same code!
     */
    public static Bytes decodeFmMfm(Bits bits)
    {
        Bytes bytes = new Bytes(0);
        ByteWriter bw = new ByteWriter(bytes);

        int bitcount = 0;
        int fifo = 0;
        int i = 0;
        while (i < bits.size())
        {
            i++; /* skip clock bit */
            if (i >= bits.size())
                break;
            fifo = (fifo << 1) | (bits.getBit(i++) ? 1 : 0);

            bitcount++;
            if (bitcount == 8)
            {
                bw.write8(fifo);
                bitcount = 0;
            }
        }

        if (bitcount != 0)
        {
            fifo <<= 8 - bitcount;
            bw.write8(fifo);
        }

        return bytes;
    }

    public static void encodeFm(Bits bits, int[] cursor, Bytes input)
    {
        if (bits.size() == 0)
            return;
        int len = bits.size() - 1;

        for (int i = 0; i < input.size(); i++)
        {
            int b = input.getByte(i) & 0xff;
            for (int j = 0; j < 8; j++)
            {
                boolean bit = (b & 0x80) != 0;
                b <<= 1;

                if (cursor[0] >= len)
                    return;

                bits.set(cursor[0]++, true);
                bits.set(cursor[0]++, bit);
            }
        }
    }

    public static void encodeMfm(
        Bits bits, int[] cursor, Bytes data, boolean[] lastBit)
    {
        if (bits.size() == 0)
            return;
        int len = bits.size() - 1;

        for (int i = 0; i < data.size(); i++)
        {
            int b = data.getByte(i) & 0xff;
            for (int j = 0; j < 8; j++)
            {
                boolean bit = (b & 0x80) != 0;
                b <<= 1;

                if (cursor[0] >= len)
                    return;

                bits.set(cursor[0]++, !lastBit[0] && !bit);
                bits.set(cursor[0]++, bit);
                lastBit[0] = bit;
            }
        }
    }

    public static Bytes encodeMfm(Bytes data, boolean[] lastBit)
    {
        ByteReader br = new ByteReader(data);
        BitReader bitr = new BitReader(br);
        Bytes out = new Bytes(0);
        ByteWriter bw = new ByteWriter(out);
        BitWriter bitw = new BitWriter(bw);

        while (bitr.hasNext())
        {
            boolean bit = bitr.next();

            bitw.push(!lastBit[0] && !bit);
            bitw.push(bit);
            lastBit[0] = bit;
        }

        bitw.flush();
        return out;
    }
}