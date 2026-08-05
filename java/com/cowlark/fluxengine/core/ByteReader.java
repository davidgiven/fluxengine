package com.cowlark.fluxengine.core;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * A cursor which reads values from a Bytes, ported from lib/core/bytes.h.
 */
public final class ByteReader implements Iterator<Byte>
{
    private final Bytes bytes;
    private int pos;

    public ByteReader(Bytes bytes)
    {
        this.bytes = bytes;
        pos = 0;
    }

    public int pos()
    {
        return pos;
    }

    public ByteReader seek(int pos)
    {
        this.pos = pos;
        return this;
    }

    public ByteReader skip(int delta)
    {
        pos += delta;
        return this;
    }

    public boolean eof()
    {
        return pos >= bytes.size();
    }

    public int remaining()
    {
        return bytes.size() - pos;
    }

    @Override
    public boolean hasNext()
    {
        return !eof();
    }

    @Override
    public Byte next()
    {
        if (!hasNext())
            throw new NoSuchElementException();
        return (byte) read8();
    }

    public Bytes read(int len)
    {
        checkReadable(len);
        Bytes slice = bytes.slice(pos, len);
        pos += len;
        return slice;
    }

    public int read8()
    {
        checkReadable(1);
        return bytes.get(pos++) & 0xff;
    }

    public int readBe16()
    {
        checkReadable(2);
        int b1 = read8();
        int b2 = read8();
        return (b1 << 8) | b2;
    }

    public int readLe16()
    {
        checkReadable(2);
        int b1 = read8();
        int b2 = read8();
        return (b2 << 8) | b1;
    }

    public int readBe24()
    {
        checkReadable(3);
        int b1 = read8();
        int b2 = read8();
        int b3 = read8();
        return (b1 << 16) | (b2 << 8) | b3;
    }

    public int readLe24()
    {
        checkReadable(3);
        int b1 = read8();
        int b2 = read8();
        int b3 = read8();
        return (b3 << 16) | (b2 << 8) | b1;
    }

    public int readBe32()
    {
        checkReadable(4);
        int b1 = read8();
        int b2 = read8();
        int b3 = read8();
        int b4 = read8();
        return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
    }

    public int readLe32()
    {
        checkReadable(4);
        int b1 = read8();
        int b2 = read8();
        int b3 = read8();
        int b4 = read8();
        return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
    }

    public long readBe48()
    {
        checkReadable(6);
        long hi = readBe16();
        long lo = readBe32() & 0xffffffffL;
        return (hi << 32) | lo;
    }

    public long readLe48()
    {
        checkReadable(6);
        long lo = readLe32() & 0xffffffffL;
        long hi = readLe16();
        return (hi << 32) | lo;
    }

    public long readBe64()
    {
        checkReadable(8);
        long hi = readBe32() & 0xffffffffL;
        long lo = readBe32() & 0xffffffffL;
        return (hi << 32) | lo;
    }

    public long readLe64()
    {
        checkReadable(8);
        long lo = readLe32() & 0xffffffffL;
        long hi = readLe32() & 0xffffffffL;
        return (hi << 32) | lo;
    }

    private void checkReadable(int len)
    {
        if (len < 0 || pos + len > bytes.size())
            throw new IndexOutOfBoundsException(String.valueOf(pos));
    }
}
