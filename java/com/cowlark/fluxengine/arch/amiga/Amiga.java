package com.cowlark.fluxengine.arch.amiga;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;

/**
 * Constants and helpers for the Amiga format, ported from arch/amiga/amiga.h
 * and arch/amiga/amiga.cc.
 */
public final class Amiga
{
    public static final long AMIGA_SECTOR_RECORD = 0xaaaa44894489L;

    public static final int AMIGA_TRACKS_PER_DISK = 80;
    public static final int AMIGA_SECTORS_PER_TRACK = 11;
    public static final int AMIGA_RECORD_SIZE = 0x21c;

    private Amiga()
    {
    }

    public static int amigaChecksum(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int checksum = 0;

        while (!br.eof())
            checksum ^= br.readBe32();

        return checksum & 0x55555555;
    }

    private static int everyother(int x)
    {
        /* aabb ccdd eeff gghh */
        x &= 0x6666; /* 0ab0 0cd0 0ef0 0gh0 */
        x >>= 1;     /* 00ab 00cd 00ef 00gh */
        x |= x << 2; /* abab cdcd efef ghgh */
        x &= 0x3c3c; /* 00ab cd00 00ef gh00 */
        x >>= 2;     /* 0000 abcd 0000 efgh */
        x |= x >> 4; /* 0000 abcd abcd efgh */
        return x;
    }

    public static Bytes amigaInterleave(Bytes input)
    {
        Bytes output = new Bytes();
        ByteWriter bw = new ByteWriter(output);

        /* Write all odd bits. (Numbering starts at 0...) */

        {
            ByteReader br = new ByteReader(input);
            while (!br.eof())
            {
                int x = br.readBe16();
                x &= 0xaaaa;       /* a0b0 c0d0 e0f0 g0h0 */
                x |= x >> 1;       /* aabb ccdd eeff gghh */
                x = everyother(x); /* 0000 0000 abcd efgh */
                bw.write8(x);
            }
        }

        /* Write all even bits. */

        {
            ByteReader br = new ByteReader(input);
            while (!br.eof())
            {
                int x = br.readBe16();
                x &= 0x5555;       /* 0a0b 0c0d 0e0f 0g0h */
                x |= x << 1;       /* aabb ccdd eeff gghh */
                x = everyother(x); /* 0000 0000 abcd efgh */
                bw.write8(x);
            }
        }

        return output;
    }

    /* Deinterleaves `len` bytes starting at `index[0]` within `input`,
     * advancing `index[0]` by `len`. Mirrors the pointer-advancing C++
     * amigaDeinterleave(). */
    public static Bytes amigaDeinterleave(Bytes input, int[] index, int len)
    {
        int start = index[0];
        int odds = start;
        int evens = start + len / 2;
        Bytes output = new Bytes();
        ByteWriter bw = new ByteWriter(output);

        for (int i = 0; i < len / 2; i++)
        {
            int o = input.getByte(odds++) & 0xff;
            int e = input.getByte(evens++) & 0xff;

            /* This is the 'Interleave bits with 64-bit multiply' technique
             * from
             * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
             */
            long result =
                    ((((e * 0x0101010101010101L) & 0x8040201008040201L) *
                                    0x0102040810204081L >>> 49) &
                            0x5555) |
                    ((((o * 0x0101010101010101L) & 0x8040201008040201L) *
                                    0x0102040810204081L >>> 48) &
                            0xAAAA);

            bw.writeBe16((int) result);
        }

        index[0] += len;
        return output;
    }

    public static Bytes amigaDeinterleave(Bytes input)
    {
        int[] index = {0};
        return amigaDeinterleave(input, index, input.size());
    }
}