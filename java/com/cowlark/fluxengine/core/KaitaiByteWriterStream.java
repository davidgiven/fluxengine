package com.cowlark.fluxengine.core;

import io.kaitai.struct.KaitaiStream;
import java.io.IOException;

public class KaitaiByteWriterStream extends KaitaiStream
{
    private final ByteWriter bw;

    public KaitaiByteWriterStream(ByteWriter bw)
    {
        this.bw = bw;
    }

    @Override
    public void close() throws IOException
    {
    }

    @Override
    public boolean isEof()
    {
        return false;
    }

    @Override
    public void seek(int i)
    {

    }

    @Override
    public void seek(long l)
    {

    }

    @Override
    public int pos()
    {
        return 0;
    }

    @Override
    public long size()
    {
        return 0;
    }

    @Override
    public byte readS1()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public short readS2be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public int readS4be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public long readS8be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public short readS2le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public int readS4le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public long readS8le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public int readU1()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public int readU2be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public long readU4be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public int readU2le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public long readU4le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public float readF4be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public double readF8be()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public float readF4le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public double readF8le()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    protected byte[] readBytesNotAligned(long l)
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public byte[] readBytesFull()
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public byte[] readBytesTerm(byte b, boolean b1, boolean b2, boolean b3)
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public byte[] readBytesTermMulti(byte[] bytes, boolean b, boolean b1, boolean b2)
    {
        throw new UnsupportedOperationException("read-only");
    }

    @Override
    public void writeS1(byte b)
    {
        bw.write8(b);
    }

    @Override
    public void writeS2be(short i)
    {
        bw.writeBe16(i);
    }

    @Override
    public void writeS4be(int i)
    {
        bw.writeBe32(i);
    }

    @Override
    public void writeS8be(long l)
    {
        bw.writeBe64(l);
    }

    @Override
    public void writeS2le(short i)
    {
        bw.writeLe16(i);
    }

    @Override
    public void writeS4le(int i)
    {
        bw.writeLe32(i);
    }

    @Override
    public void writeS8le(long l)
    {
        bw.writeLe64(l);
    }

    @Override
    public void writeF4be(float v)
    {
        bw.writeBe32(Float.floatToIntBits(v));
    }

    @Override
    public void writeF8be(double v)
    {
        bw.writeBe64(Double.doubleToLongBits(v));
    }

    @Override
    public void writeF4le(float v)
    {
        bw.writeLe32(Float.floatToIntBits(v));
    }

    @Override
    public void writeF8le(double v)
    {
        bw.writeLe64(Double.doubleToLongBits(v));
    }

    @Override
    protected void writeBytesNotAligned(byte[] bytes)
    {
        bw.write(bytes);
    }

    @Override
    public KaitaiStream substream(long l)
    {
        return null;
    }
}