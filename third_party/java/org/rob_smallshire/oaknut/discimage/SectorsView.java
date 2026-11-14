package org.rob_smallshire.oaknut.discimage;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * A write-through view over possibly noncontiguous storage ranges. Building or slicing a view
 * performs no I/O. Storage failures use UncheckedIOException to preserve the buffer-view API. */
public final class SectorsView
{
    private final List<Span> spans;
    private final int length;
    private SectorsView(List<Span> spans, int length)
    {
        this.spans = List.copyOf(spans);
        this.length = length;
    }

    public SectorsView(List<ByteBuffer> buffers)
    {
        this(
                buffers
                        .stream()
                        .map(b -> new Span(new BufferStorage(b), 0, b.remaining()))
                        .toList(),
                buffers.stream().mapToInt(ByteBuffer::remaining).reduce(0, Math::addExact));
    }

    public SectorsView(ByteBuffer buffer)
    {
        this(List.of(buffer));
    }

    public SectorsView(Storage storage, int offset, int length)
    {
        Objects.checkFromIndexSize(offset, length, storage.size());
        spans = List.of(new Span(storage, offset, length));
        this.length = length;
    }

    public static SectorsView wrap(byte[] bytes)
    {
        return new SectorsView(ByteBuffer.wrap(bytes));
    }

    public static SectorsView join(List<SectorsView> views)
    {
        ArrayList<Span> spans = new ArrayList<>();
        int length = 0;
        for (SectorsView view : views)
        {
            spans.addAll(view.spans);
            length = Math.addExact(length, view.length);
        }
        return new SectorsView(spans, length);
    }

    public int size()
    {
        return length;
    }

    public boolean isReadOnly()
    {
        return spans.stream().anyMatch(s -> s.storage().isReadOnly());
    }

    public int get(int index)
    {
        Objects.checkIndex(index, length);
        for (Span span : spans)
        {
            if (index < span.length())
            {
                byte[] value = new byte[1];
                try
                {
                    span.storage().read(span.offset() + index, value, 0, 1);
                } catch (IOException e)
                {
                    throw new UncheckedIOException(e);
                }
                return value[0] & 255;
            }
            index -= span.length();
        }
        throw new IndexOutOfBoundsException();
    }

    public void put(int index, int value)
    {
        if (value < 0 || value > 255)
            throw new IllegalArgumentException("Byte outside 0..255");
        Objects.checkIndex(index, length);
        for (Span span : spans)
        {
            if (index < span.length())
            {
                if (span.storage().isReadOnly())
                    throw new java.nio.ReadOnlyBufferException();
                try
                {
                    span.storage().write(span.offset() + index, new byte[]{(byte) value}, 0, 1);
                } catch (IOException e)
                {
                    throw new UncheckedIOException(e);
                }
                return;
            }
            index -= span.length();
        }
    }

    public SectorsView slice(int offset, int size)
    {
        Objects.checkFromIndexSize(offset, size, length);
        ArrayList<Span> result = new ArrayList<>();
        int remaining = size;
        for (Span span : spans)
        {
            int take = Math.min(remaining, Math.max(0, span.length() - offset));
            if (take > 0)
            {
                result.add(new Span(span.storage(), span.offset() + offset, take));
                remaining -= take;
            }
            offset = Math.max(0, offset - span.length());
            if (remaining == 0)
                break;
        }
        return new SectorsView(result, size);
    }

    public byte[] read(int offset, int size)
    {
        SectorsView slice = slice(offset, size);
        byte[] result = new byte[size];
        int cursor = 0;
        try
        {
            for (Span span : slice.spans)
            {
                span.storage().read(span.offset(), result, cursor, span.length());
                cursor += span.length();
            }
        } catch (IOException e)
        {
            throw new UncheckedIOException(e);
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
        try
        {
            for (Span span : slice.spans)
            {
                span.storage().write(span.offset(), bytes, cursor, span.length());
                cursor += span.length();
            }
        } catch (IOException e)
        {
            throw new UncheckedIOException(e);
        }
    }

    /* Integer helpers preserve unsigned on-disc values without changing buffer positions. */
    public long readLe(int offset, int size)
    {
        byte[] bytes = read(offset, size);
        long value = 0;
        for (int i = 0; i < size; i++)
            value |= (long) (bytes[i] & 255) << (8 * i);
        return value;
    }

    public void writeLe(int offset, long value, int size)
    {
        byte[] bytes = new byte[size];
        for (int i = 0; i < size; i++)
            bytes[i] = (byte) (value >>> (8 * i));
        write(offset, bytes);
    }

    private record Span(Storage storage, int offset, int length)
    {
    }
}
