package com.cowlark.fluxengine.core;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cursor which reads bits from a ByteReader.
 */
public final class BitReader implements Iterator<Boolean>
{
    private final ByteReader reader;
    private int fifo;
    private int bitcount;

    public BitReader(ByteReader reader)
    {
        this.reader = reader;
    }

    public boolean get()
    {
        if (bitcount == 0)
            fifo = reader.read8();

        boolean bit = (fifo & 0x80) != 0;
        fifo <<= 1;
        bitcount = (bitcount + 1) & 7;
        return bit;
    }

    public boolean eof()
    {
        return bitcount == 0 && reader.eof();
    }

    @Override
    public boolean hasNext()
    {
        return !eof();
    }

    @Override
    public Boolean next()
    {
        if (!hasNext())
            throw new NoSuchElementException();
        return get();
    }
}
