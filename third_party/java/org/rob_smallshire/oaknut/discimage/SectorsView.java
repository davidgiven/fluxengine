package org.rob_smallshire.oaknut.discimage;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Objects;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * A write-through view over possibly noncontiguous sector buffers. */
public final class SectorsView
{
    private final List<ByteBuffer> buffers;
    private final int length;

    public SectorsView(List<ByteBuffer> buffers)
    {
        this.buffers = buffers.stream().map(ByteBuffer::slice).toList();
        this.length = this.buffers.stream().mapToInt(ByteBuffer::capacity).sum();
    }

    public SectorsView(ByteBuffer buffer)
    {
        this(List.of(buffer));
    }

    public static SectorsView wrap(byte[] bytes)
    {
        return new SectorsView(ByteBuffer.wrap(bytes));
    }

    public int size()
    {
        return length;
    }

    public boolean isReadOnly()
    {
        return buffers.stream().anyMatch(ByteBuffer::isReadOnly);
    }

    public int get(int index)
    {
        Objects.checkIndex(index, length);
        for (ByteBuffer buffer : buffers)
        {
            if (index < buffer.capacity())
                return buffer.get(index) & 255;
            index -= buffer.capacity();
        }
        throw new IndexOutOfBoundsException();
    }

    public void put(int index, int value)
    {
        Objects.checkIndex(index, length);
        if (value < 0 || value > 255)
            throw new IllegalArgumentException("Byte outside 0..255");
        for (ByteBuffer buffer : buffers)
        {
            if (index < buffer.capacity())
            {
                buffer.put(index, (byte) value);
                return;
            }
            index -= buffer.capacity();
        }
    }

    public SectorsView slice(int offset, int size)
    {
        Objects.checkFromIndexSize(offset, size, length);
        java.util.ArrayList<ByteBuffer> result = new java.util.ArrayList<>();
        for (ByteBuffer buffer : buffers)
        {
            int take = Math.min(size, Math.max(0, buffer.capacity() - offset));
            if (take > 0)
            {
                result.add(buffer.slice(offset, take));
                size -= take;
            }
            offset = Math.max(0, offset - buffer.capacity());
        }
        return new SectorsView(result);
    }

    public byte[] read(int offset, int size)
    {
        Objects.checkFromIndexSize(offset, size, length);
        byte[] result = new byte[size];
        SectorsView slice = slice(offset, size);
        int cursor = 0;
        for (ByteBuffer buffer : slice.buffers)
        {
            buffer.get(0, result, cursor, buffer.capacity());
            cursor += buffer.capacity();
        }
        return result;
    }

    public byte[] toByteArray()
    {
        return read(0, length);
    }

    public void write(int offset, byte[] bytes)
    {
        SectorsView slice = slice(offset, bytes.length);
        if (slice.isReadOnly())
            throw new java.nio.ReadOnlyBufferException();
        int cursor = 0;
        for (ByteBuffer buffer : slice.buffers)
        {
            buffer.put(0, bytes, cursor, buffer.capacity());
            cursor += buffer.capacity();
        }
    }

    /* Integer helpers preserve unsigned on-disc values without changing buffer positions. */
    public long readLe(int offset, int size)
    {
        Objects.checkFromIndexSize(offset, size, length);
        long value = 0;
        for (int i = 0; i < size; i++)
            value |= (long) get(offset + i) << (8 * i);
        return value;
    }

    public void writeLe(int offset, long value, int size)
    {
        Objects.checkFromIndexSize(offset, size, length);
        for (int i = 0; i < size; i++)
            put(offset + i, (int) (value >>> (8 * i)) & 255);
    }
}
