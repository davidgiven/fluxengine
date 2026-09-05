package com.cowlark.fluxengine.core;

/**
 * A cursor which writes values into a Bytes, ported from lib/core/bytes.h.
 */
public final class ByteWriter
{
    private final Bytes bytes;
    private int pos;

    public ByteWriter(Bytes bytes)
    {
        this.bytes = bytes;
        pos = 0;
    }

    public int pos()
    {
        return pos;
    }

    public ByteWriter seek(int pos)
    {
        this.pos = pos;
        return this;
    }

    public ByteWriter seekToEnd()
    {
        pos = bytes.size();
        return this;
    }

    public ByteWriter skip(int delta)
    {
        pos += delta;
        return this;
    }

    public ByteWriter write8(int value)
    {
        ensureWritable(1);
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeBe16(int value)
    {
        ensureWritable(2);
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeLe16(int value)
    {
        ensureWritable(2);
        bytes.setByte(pos++, (byte) value);
        bytes.setByte(pos++, (byte) (value >> 8));
        return this;
    }

    public ByteWriter writeBe24(int value)
    {
        ensureWritable(3);
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeLe24(int value)
    {
        ensureWritable(3);
        bytes.setByte(pos++, (byte) value);
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) (value >> 16));
        return this;
    }

    public ByteWriter writeBe32(int value)
    {
        ensureWritable(4);
        bytes.setByte(pos++, (byte) (value >> 24));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeLe32(int value)
    {
        ensureWritable(4);
        bytes.setByte(pos++, (byte) value);
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 24));
        return this;
    }

    public ByteWriter writeBe48(long value)
    {
        ensureWritable(6);
        bytes.setByte(pos++, (byte) (value >> 40));
        bytes.setByte(pos++, (byte) (value >> 32));
        bytes.setByte(pos++, (byte) (value >> 24));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeLe48(long value)
    {
        ensureWritable(6);
        bytes.setByte(pos++, (byte) value);
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 24));
        bytes.setByte(pos++, (byte) (value >> 32));
        bytes.setByte(pos++, (byte) (value >> 40));
        return this;
    }

    public ByteWriter writeBe64(long value)
    {
        ensureWritable(8);
        bytes.setByte(pos++, (byte) (value >> 56));
        bytes.setByte(pos++, (byte) (value >> 48));
        bytes.setByte(pos++, (byte) (value >> 40));
        bytes.setByte(pos++, (byte) (value >> 32));
        bytes.setByte(pos++, (byte) (value >> 24));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) value);
        return this;
    }

    public ByteWriter writeLe64(long value)
    {
        ensureWritable(8);
        bytes.setByte(pos++, (byte) value);
        bytes.setByte(pos++, (byte) (value >> 8));
        bytes.setByte(pos++, (byte) (value >> 16));
        bytes.setByte(pos++, (byte) (value >> 24));
        bytes.setByte(pos++, (byte) (value >> 32));
        bytes.setByte(pos++, (byte) (value >> 40));
        bytes.setByte(pos++, (byte) (value >> 48));
        bytes.setByte(pos++, (byte) (value >> 56));
        return this;
    }

    public ByteWriter write(Bytes data)
    {
        ensureWritable(data.size());
        for (int i = 0; i < data.size(); i++)
            bytes.setByte(pos++, data.get(i));
        return this;
    }

    public ByteWriter write(byte[] data)
    {
        ensureWritable(data.length);
        for (byte b : data)
            bytes.setByte(pos++, b);
        return this;
    }

    public ByteWriter pad(int count)
    {
        return pad(count, 0);
    }

    public ByteWriter pad(int count, int value)
    {
        ensureWritable(count);
        for (int i = 0; i < count; i++)
            bytes.setByte(pos++, (byte) value);
        return this;
    }

    private void ensureWritable(int width)
    {
        if (pos + width > bytes.size())
            bytes.resize(pos + width);
    }
}
