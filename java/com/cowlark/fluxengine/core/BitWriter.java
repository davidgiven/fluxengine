package com.cowlark.fluxengine.core;

/**
 * A cursor which packs bits into a ByteWriter.
 */
public final class BitWriter
{
    private final ByteWriter writer;
    private int fifo;
    private int bitcount;

    public BitWriter(ByteWriter writer)
    {
        this.writer = writer;
    }

    public BitWriter push(int bits, int size)
    {
        bits <<= 32 - size;

        while (size-- != 0)
        {
            fifo = (fifo << 1) | (bits >>> 31);
            bitcount++;
            bits <<= 1;
            if (bitcount == 8)
            {
                writer.write8(fifo);
                bitcount = 0;
                fifo = 0;
            }
        }
        return this;
    }

    public BitWriter push(boolean bit)
    {
        return push(bit ? 1 : 0, 1);
    }

    public void flush()
    {
        if (bitcount != 0)
        {
            writer.write8(fifo);
            bitcount = 0;
        }
    }
}
