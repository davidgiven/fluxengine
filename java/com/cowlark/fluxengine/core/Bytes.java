package com.cowlark.fluxengine.core;

import java.nio.charset.StandardCharsets;

/**
 * A resizable byte container, ported from lib/core/bytes.h. Slices are
 * read-only views over the shared backing storage.
 */
public final class Bytes
{
    private static final class Storage
    {
        byte[] data;

        Storage(int capacity)
        {
            data = new byte[capacity];
        }
    }

    private Storage storage;
    private int low;
    private int high;
    private boolean readOnly;

    public Bytes()
    {
        this(0);
    }

    public Bytes(int size)
    {
        storage = new Storage(size);
        low = 0;
        high = size;
    }

    public Bytes(byte[] data)
    {
        this(data.length);
        System.arraycopy(data, 0, storage.data, 0, data.length);
    }

    public Bytes(String data)
    {
        this(data.getBytes(StandardCharsets.UTF_8));
    }

    public static Bytes of(int... values)
    {
        byte[] data = new byte[values.length];
        for (int i = 0; i < values.length; i++)
            data[i] = (byte) values[i];
        return new Bytes(data);
    }

    private Bytes(Storage storage, int low, int high)
    {
        this.storage = storage;
        this.low = low;
        this.high = high;
        readOnly = true;
    }

    public int size()
    {
        return high - low;
    }

    public boolean isEmpty()
    {
        return high == low;
    }

    public byte get(int offset)
    {
        boundsCheck(offset);
        return storage.data[low + offset];
    }

    public void set(int offset, byte value)
    {
        checkWritable();
        boundsCheck(offset);
        storage.data[low + offset] = value;
    }

    public byte[] toArray()
    {
        byte[] result = new byte[size()];
        System.arraycopy(storage.data, low, result, 0, result.length);
        return result;
    }

    public void resize(int newSize)
    {
        checkWritable();
        ensureCapacity(low + newSize);
        high = low + newSize;
    }

    public Bytes slice(int start, int len)
    {
        if (start < 0 || len < 0 || start + len > size())
            throw new IndexOutOfBoundsException();
        return new Bytes(storage, low + start, low + start + len);
    }

    public Bytes concat(Bytes other)
    {
        Bytes result = new Bytes(size() + other.size());
        System.arraycopy(storage.data, low, result.storage.data, 0, size());
        System.arraycopy(other.storage.data, other.low, result.storage.data,
            size(), other.size());
        return result;
    }

    public Bytes repeat(int count)
    {
        Bytes result = new Bytes(size() * count);
        for (int i = 0; i < count; i++)
            System.arraycopy(storage.data, low, result.storage.data, i * size(),
                size());
        return result;
    }

    byte[] array()
    {
        return storage.data;
    }

    @Override
    public boolean equals(Object o)
    {
        if (!(o instanceof Bytes))
            return false;
        Bytes other = (Bytes) o;
        if (size() != other.size())
            return false;
        for (int i = 0; i < size(); i++)
        {
            if (storage.data[low + i] != other.storage.data[other.low + i])
                return false;
        }
        return true;
    }

    @Override
    public int hashCode()
    {
        int hash = 1;
        for (int i = 0; i < size(); i++)
            hash = 31 * hash + storage.data[low + i];
        return hash;
    }

    @Override
    public String toString()
    {
        return String.format("Bytes(hash=%08x, readOnly=%s, size=%d)",
            System.identityHashCode(this), readOnly, size());
    }

    private void checkWritable()
    {
        if (readOnly)
            throw new UnsupportedOperationException("slice is read-only");
    }

    private void boundsCheck(int offset)
    {
        if (offset < 0 || offset >= size())
            throw new IndexOutOfBoundsException(String.valueOf(offset));
    }

    private void ensureCapacity(int capacity)
    {
        if (capacity <= storage.data.length)
            return;
        int newCapacity = Math.max(capacity, storage.data.length * 2);
        byte[] newData = new byte[newCapacity];
        System.arraycopy(storage.data, 0, newData, 0, storage.data.length);
        storage.data = newData;
    }
}
