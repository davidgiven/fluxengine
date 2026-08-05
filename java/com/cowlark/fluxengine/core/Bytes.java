package com.cowlark.fluxengine.core;

import com.google.common.collect.ImmutableList;
import java.nio.charset.StandardCharsets;

/**
 * A resizable byte container, ported from lib/core/bytes.h. Slices share the
 * parent's storage; writes to a shared storage copy it first, so changes to
 * one window are invisible to the others.
 */
public final class Bytes implements Iterable<Byte>
{
    private static final class Storage
    {
        byte[] data;
        int refcount;

        Storage(int capacity)
        {
            data = new byte[capacity];
            refcount = 1;
        }
    }

    private Storage storage;
    private int low;
    private int high;

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
        storage.refcount++;
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
        boundsCheck(offset);
        detach();
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
        detach();
        ensureCapacity(low + newSize);
        high = low + newSize;
    }

    public Bytes slice(int start, int len)
    {
        if (start < 0 || len < 0)
            throw new IndexOutOfBoundsException();
        if (start >= size())
            return new Bytes(len);
        int available = Math.min(len, size() - start);
        if (available < len)
        {
            Bytes result = new Bytes(len);
            System.arraycopy(storage.data, low + start, result.storage.data, 0,
                available);
            return result;
        }
        return new Bytes(storage, low + start, low + start + len);
    }

    public Bytes slice(int start)
    {
        int len = 0;
        if (start < size())
            len = size() - start;
        return slice(start, len);
    }

    public void clear()
    {
        resize(0);
    }

    public ImmutableList<Bytes> split(int separator)
    {
        ImmutableList.Builder<Bytes> pieces = ImmutableList.builder();
        int lastEnd = 0;
        for (int i = 0; i < size(); i++)
        {
            if ((get(i) & 0xff) == separator)
            {
                pieces.add(slice(lastEnd, i - lastEnd));
                lastEnd = i + 1;
            }
        }
        pieces.add(slice(lastEnd));
        return pieces.build();
    }

    public Bytes swab()
    {
        Bytes output = new Bytes(0);
        ByteWriter bw = new ByteWriter(output);
        ByteReader br = new ByteReader(this);
        while (!br.eof())
        {
            int a = br.read8();
            int b = br.eof() ? 0 : br.read8();
            bw.write8(b);
            bw.write8(a);
        }
        return output;
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

    int refcount()
    {
        return storage.refcount;
    }

    @Override
    public ByteReader iterator()
    {
        return new ByteReader(this);
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
        return String.format("Bytes(hash=%08x, refcount=%d, size=%d)",
            System.identityHashCode(this), storage.refcount, size());
    }

    /* Copy-on-write: if this window shares its storage with other windows,
     * detach it into a private copy so mutations don't affect them. */
    private void detach()
    {
        if (storage.refcount > 1)
        {
            Storage old = storage;
            int size = size();
            Storage fresh = new Storage(size);
            System.arraycopy(old.data, low, fresh.data, 0, size);
            storage = fresh;
            low = 0;
            high = size;
            old.refcount--;
        }
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
