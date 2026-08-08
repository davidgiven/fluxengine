package com.cowlark.fluxengine.core;

import java.util.AbstractList;
import java.util.BitSet;

/**
 * A packed list of booleans backed by a java.util.BitSet, the Java equivalent
 * of std::vector<bool>. The logical size is tracked separately, so trailing
 * falses are part of the list.
 */
public final class Bits extends AbstractList<Boolean>
{
    private final BitSet bits = new BitSet();
    private int size;

    public Bits()
    {
    }

    public Bits(int size)
    {
        this.size = size;
    }

    @Override
    public int size()
    {
        return size;
    }

    @Override
    public Boolean get(int index)
    {
        return getBit(index);
    }

    /* Fast, allocation-free bit access for hot paths. */
    public boolean getBit(int index)
    {
        checkIndex(index);
        return bits.get(index);
    }

    @Override
    public Boolean set(int index, Boolean value)
    {
        checkIndex(index);
        boolean old = bits.get(index);
        bits.set(index, value);
        return old;
    }

    /* Fast, allocation-free bit write for hot paths. */
    public void setBit(int index, boolean value)
    {
        checkIndex(index);
        bits.set(index, value);
    }

    @Override
    public boolean add(Boolean value)
    {
        bits.set(size, value);
        size++;
        modCount++;
        return true;
    }

    @Override
    public void add(int index, Boolean value)
    {
        if (index < 0 || index > size)
            throw new IndexOutOfBoundsException(String.valueOf(index));
        for (int i = size; i > index; i--)
            bits.set(i, bits.get(i - 1));
        bits.set(index, value);
        size++;
        modCount++;
    }

    @Override
    public Boolean remove(int index)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean remove(Object o)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public void clear()
    {
        bits.clear();
        size = 0;
        modCount++;
    }

    /* Returns a new Bits with the bits in reverse order. */
    public Bits reverseBits()
    {
        Bits result = new Bits(size);
        for (int i = 0; i < size; i++)
            result.setBit(size - 1 - i, getBit(i));
        return result;
    }

    /* Packs the bits MSB-first into a Bytes (the inverse of Bytes.toBits). */
    public Bytes toBytes()
    {
        Bytes bytes = new Bytes(0);
        BitWriter bitw = new BitWriter(new ByteWriter(bytes));
        for (int i = 0; i < size; i++)
            bitw.push(getBit(i));
        bitw.flush();
        return bytes;
    }

    /* Fills this Bits from the cursor's current position up to (but not
     * including) terminateAt with the given pattern, advancing the cursor. */
    public void fillBitmapTo(Cursor cursor, int terminateAt, boolean[] pattern)
    {
        while (cursor.get() < terminateAt)
        {
            for (boolean b : pattern)
            {
                if (cursor.get() < size)
                {
                    setBit(cursor.get(), b);
                    cursor.advance();
                }
            }
        }
    }

    private void checkIndex(int index)
    {
        if (index < 0 || index >= size)
            throw new IndexOutOfBoundsException(String.valueOf(index));
    }

    /**
     * A mutable cursor into a {@link Bits}, providing the in/out semantics of the
     * C++ {@code unsigned& cursor} parameter passed to the bit-writing helpers.
     * The current position is held directly, so a single cursor can be shared and
     * advanced by successive calls.
     */
    public static final class Cursor
    {
        private int index;

        public Cursor(int index)
        {
            this.index = index;
        }

        public int get()
        {
            return index;
        }

        public void set(int value)
        {
            index = value;
        }

        public void advance()
        {
            index++;
        }

        public void advance(int delta)
        {
            index += delta;
        }
    }
}
