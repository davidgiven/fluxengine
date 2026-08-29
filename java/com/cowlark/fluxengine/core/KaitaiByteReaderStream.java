package com.cowlark.fluxengine.core;

import io.kaitai.struct.KaitaiStream;
import java.util.Arrays;

public class KaitaiByteReaderStream extends KaitaiStream
{
    private final ByteReader br;

    public KaitaiByteReaderStream(ByteReader br)
    {
        this.br = br;
    }

    @Override
    public void close()
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
        return (byte) br.read8();
    }

    @Override
    public short readS2be()
    {
        return (short) br.readBe16();
    }

    @Override
    public int readS4be()
    {
        return br.readBe32();
    }

    @Override
    public long readS8be()
    {
        return br.readBe64();
    }

    @Override
    public short readS2le()
    {
        return (short) br.readLe16();
    }

    @Override
    public int readS4le()
    {
        return br.readLe32();
    }

    @Override
    public long readS8le()
    {
        return br.readLe64();
    }

    @Override
    public int readU1()
    {
        return br.read8() & 0xFF;
    }

    @Override
    public int readU2be()
    {
        return br.readBe16() & 0xFFFF;
    }

    @Override
    public long readU4be()
    {
        return br.readBe32() & 0xFFFFFFFFL;
    }

    @Override
    public int readU2le()
    {
        return br.readLe16() & 0xFFFF;
    }

    @Override
    public long readU4le()
    {
        return br.readLe32() & 0xFFFFFFFFL;
    }

    @Override
    public float readF4be()
    {
        return Float.intBitsToFloat(br.readBe32());
    }

    @Override
    public double readF8be()
    {
        return Double.longBitsToDouble(br.readBe64());
    }

    @Override
    public float readF4le()
    {
        return Float.intBitsToFloat(br.readLe32());
    }

    @Override
    public double readF8le()
    {
        return Double.longBitsToDouble(br.readLe64());
    }

    @Override
    protected byte[] readBytesNotAligned(long l)
    {
        Bytes slice = br.read((int) l);
        return slice.toByteArray();
    }

    @Override
    public byte[] readBytesFull()
    {
        Bytes all = br.read(br.remaining());
        return all.toByteArray();
    }

    @Override
    public byte[] readBytesTerm(
            byte term,
            boolean includeTerm,
            boolean consumeTerm,
            boolean eosError)
    {
        ByteReader reader = br;
        byte[] result = new byte[0];

        while (true)
        {
            if (reader.remaining() == 0)
            {
                if (eosError)
                    throw new RuntimeException("End of stream reached, but no terminator found");
                return result;
            }

            byte b = (byte) reader.read8();
            if (b == term)
            {
                // Do not include the terminator in the result
                // Position already advanced past the terminator by read8()
                if (!includeTerm)
                    return result;

                // Position already advanced past the terminator by read8()
                return appendByte(result, b);
            }
            result = appendByte(result, b);
        }
    }

    @Override
    public byte[] readBytesTermMulti(
            byte[] term,
            boolean includeTerm,
            boolean consumeTerm,
            boolean eosError)
    {
        ByteReader reader = br;
        byte[] result = new byte[0];

        // Read all remaining bytes
        while (reader.remaining() > 0)
        {
            byte b = (byte) reader.read8();
            result = appendByte(result, b);
        }

        // After stream ended, check if result ends with the terminator
        if (result.length >= term.length)
        {
            boolean match = true;
            for (int i = 0; i < term.length; i++)
            {
                if (result[result.length - term.length + i] != term[i])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                if (!includeTerm)
                    result = Arrays.copyOf(result, result.length - term.length);
                return result;
            } else if (eosError)
                throw new RuntimeException("End of stream reached, but no terminator found");
        } else if (eosError)
        {
            throw new RuntimeException("End of stream reached, but no terminator found");
        }

        return result;
    }

    @Override
    protected void writeBytesNotAligned(byte[] bytes)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS1(byte b)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS2be(short i)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS4be(int i)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS8be(long l)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS2le(short i)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS4le(int i)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeS8le(long l)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeF4be(float v)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeF8be(double v)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeF4le(float v)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public void writeF8le(double v)
    {
        throw new UnsupportedOperationException("write-only");
    }

    @Override
    public KaitaiStream substream(long n)
    {
        // Read exactly n bytes from the current position and create a new stream
        Bytes slice = br.read((int) n);
        ByteReader newBr = new ByteReader(slice);
        return new KaitaiByteReaderStream(newBr);
    }

    private byte[] appendByte(byte[] result, byte b)
    {
        byte[] newResult = new byte[result.length + 1];
        System.arraycopy(result, 0, newResult, 0, result.length);
        newResult[result.length] = b;
        return newResult;
    }

    private boolean arraysEqual(byte[] a, byte[] b)
    {
        if (a.length != b.length)
            return false;
        for (int i = 0; i < a.length; i++)
        {
            if (a[i] != b[i])
                return false;
        }
        return true;
    }
}