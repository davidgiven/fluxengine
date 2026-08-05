package com.cowlark.fluxengine.core;

import com.google.common.collect.ImmutableList;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.ListIterator;
import java.util.NoSuchElementException;
import java.util.zip.DataFormatException;
import java.util.zip.Deflater;
import java.util.zip.Inflater;

/**
 * A resizable byte container, ported from lib/core/bytes.h. Slices share the
 * parent's storage; writes to a shared storage copy it first, so changes to
 * one window are invisible to the others.
 */
public final class Bytes implements List<Byte>
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

    @Override
    public Byte get(int offset)
    {
        return getByte(offset);
    }

    /* Fast, allocation-free byte access for hot paths (avoids Byte boxing). */
    public byte getByte(int offset)
    {
        boundsCheck(offset);
        return storage.data[low + offset];
    }

    @Override
    public Byte set(int offset, Byte value)
    {
        boundsCheck(offset);
        detach();
        byte old = storage.data[low + offset];
        storage.data[low + offset] = value;
        return old;
    }

    /* Fast, allocation-free byte write for hot paths (avoids Byte boxing). */
    public void setByte(int offset, byte value)
    {
        boundsCheck(offset);
        detach();
        storage.data[low + offset] = value;
    }

    public byte[] toByteArray()
    {
        byte[] result = new byte[size()];
        System.arraycopy(storage.data, low, result, 0, result.length);
        return result;
    }

    @Override
    public Object[] toArray()
    {
        Object[] result = new Object[size()];
        for (int i = 0; i < size(); i++)
            result[i] = getByte(i);
        return result;
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T[] toArray(T[] a)
    {
        int n = size();
        if (a.length < n)
            a = (T[]) Arrays.copyOf(a, n, a.getClass());
        for (int i = 0; i < n; i++)
            a[i] = (T) Byte.valueOf(getByte(i));
        if (a.length > n)
            a[n] = null;
        return a;
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
            if ((getByte(i) & 0xff) == separator)
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

    /* Produces zlib-format (RFC 1950) data, compatible with the C++ zlib
     * compress(). */
    public Bytes compress()
    {
        Deflater deflater = new Deflater();
        deflater.setInput(storage.data, low, size());
        deflater.finish();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        while (!deflater.finished())
        {
            int n = deflater.deflate(buffer);
            out.write(buffer, 0, n);
        }
        deflater.end();
        return new Bytes(out.toByteArray());
    }

    /* Consumes zlib-format (RFC 1950) data, compatible with the C++ zlib
     * uncompress(). */
    public Bytes decompress()
    {
        Inflater inflater = new Inflater();
        inflater.setInput(storage.data, low, size());
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        try
        {
            while (true)
            {
                int n = inflater.inflate(buffer);
                if (n > 0)
                    out.write(buffer, 0, n);
                if (inflater.finished())
                    break;
                if (n == 0)
                    throw new RuntimeException("failed to decompress data");
            }
        }
        catch (DataFormatException e)
        {
            throw new RuntimeException(
                "failed to decompress data: " + e.getMessage());
        }
        finally
        {
            inflater.end();
        }
        return new Bytes(out.toByteArray());
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
    public boolean add(Byte value)
    {
        detach();
        ensureCapacity(high + 1);
        storage.data[high] = value;
        high++;
        return true;
    }

    @Override
    public void add(int index, Byte value)
    {
        if (index < 0 || index > size())
            throw new IndexOutOfBoundsException(String.valueOf(index));
        detach();
        ensureCapacity(high + 1);
        System.arraycopy(storage.data, low + index, storage.data, low + index + 1,
            size() - index);
        storage.data[low + index] = value;
        high++;
    }

    @Override
    public Byte remove(int index)
    {
        if (index < 0 || index >= size())
            throw new IndexOutOfBoundsException(String.valueOf(index));
        detach();
        byte old = storage.data[low + index];
        System.arraycopy(storage.data, low + index + 1, storage.data,
            low + index, size() - index - 1);
        high--;
        return old;
    }

    @Override
    public boolean remove(Object o)
    {
        int index = indexOf(o);
        if (index < 0)
            return false;
        remove(index);
        return true;
    }

    @Override
    public int indexOf(Object o)
    {
        if (!(o instanceof Byte))
            return -1;
        byte target = (Byte) o;
        for (int i = 0; i < size(); i++)
        {
            if (storage.data[low + i] == target)
                return i;
        }
        return -1;
    }

    @Override
    public int lastIndexOf(Object o)
    {
        if (!(o instanceof Byte))
            return -1;
        byte target = (Byte) o;
        for (int i = size() - 1; i >= 0; i--)
        {
            if (storage.data[low + i] == target)
                return i;
        }
        return -1;
    }

    @Override
    public ListIterator<Byte> listIterator()
    {
        return listIterator(0);
    }

    @Override
    public ListIterator<Byte> listIterator(final int index)
    {
        if (index < 0 || index > size())
            throw new IndexOutOfBoundsException(String.valueOf(index));
        return new ListIterator<Byte>()
        {
            private int cursor = index;

            @Override
            public boolean hasNext()
            {
                return cursor < size();
            }

            @Override
            public Byte next()
            {
                if (!hasNext())
                    throw new NoSuchElementException();
                return get(cursor++);
            }

            @Override
            public boolean hasPrevious()
            {
                return cursor > 0;
            }

            @Override
            public Byte previous()
            {
                if (!hasPrevious())
                    throw new NoSuchElementException();
                return get(--cursor);
            }

            @Override
            public int nextIndex()
            {
                return cursor;
            }

            @Override
            public int previousIndex()
            {
                return cursor - 1;
            }

            @Override
            public void remove()
            {
                throw new UnsupportedOperationException();
            }

            @Override
            public void set(Byte value)
            {
                throw new UnsupportedOperationException();
            }

            @Override
            public void add(Byte value)
            {
                throw new UnsupportedOperationException();
            }
        };
    }

    @Override
    public List<Byte> subList(int fromIndex, int toIndex)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean contains(Object o)
    {
        return indexOf(o) >= 0;
    }

    @Override
    public boolean containsAll(Collection<?> c)
    {
        for (Object o : c)
        {
            if (!contains(o))
                return false;
        }
        return true;
    }

    @Override
    public boolean addAll(Collection<? extends Byte> c)
    {
        for (Byte b : c)
            add(b);
        return !c.isEmpty();
    }

    @Override
    public boolean addAll(int index, Collection<? extends Byte> c)
    {
        if (c.isEmpty())
            return false;
        for (Byte b : c)
            add(index++, b);
        return true;
    }

    @Override
    public boolean removeAll(Collection<?> c)
    {
        boolean changed = false;
        for (int i = size() - 1; i >= 0; i--)
        {
            if (c.contains(getByte(i)))
            {
                remove(i);
                changed = true;
            }
        }
        return changed;
    }

    @Override
    public boolean retainAll(Collection<?> c)
    {
        boolean changed = false;
        for (int i = size() - 1; i >= 0; i--)
        {
            if (!c.contains(getByte(i)))
            {
                remove(i);
                changed = true;
            }
        }
        return changed;
    }

    @Override
    public boolean equals(Object o)
    {
        if (o instanceof Bytes)
        {
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
        if (o instanceof List)
            return o.equals(this);
        return false;
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
