package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;

@RunWith(JUnit4.class)
public class BitsTest
{
    @Test
    public void basicGetSet()
    {
        Bits bits = new Bits(5);
        assertThat(bits.size()).isEqualTo(5);
        assertThat(bits.get(0)).isFalse();
        assertThat(bits.get(4)).isFalse();

        assertThat(bits.set(2, true)).isFalse();
        bits.setBit(4, true);

        assertThat(bits.get(2)).isTrue();
        assertThat(bits.getBit(4)).isTrue();
        assertThat(bits.get(0)).isFalse();
        assertThat(bits.set(2, false)).isTrue();
    }

    @Test
    public void add()
    {
        Bits bits = new Bits(0);
        bits.add(true);
        bits.add(false);
        bits.add(true);

        assertThat(bits.size()).isEqualTo(3);
        assertThat(bits.get(0)).isTrue();
        assertThat(bits.get(1)).isFalse();
        assertThat(bits.get(2)).isTrue();
    }

    @Test
    public void insert()
    {
        Bits bits = new Bits(3);
        bits.add(1, true);

        assertThat(bits.size()).isEqualTo(4);
        assertThat(bits.get(0)).isFalse();
        assertThat(bits.get(1)).isTrue();
        assertThat(bits.get(2)).isFalse();
        assertThat(bits.get(3)).isFalse();
    }

    @Test
    public void removeThrows()
    {
        Bits bits = new Bits(2);

        assertThrows(UnsupportedOperationException.class, () -> bits.remove(0));
        assertThrows(UnsupportedOperationException.class, () -> bits.remove(Boolean.TRUE));
    }

    @Test
    public void clear()
    {
        Bits bits = new Bits(4);
        bits.set(1, true);
        bits.clear();
        assertThat(bits.size()).isEqualTo(0);
    }

    @Test
    public void iteration()
    {
        Bits bits = new Bits(0);
        bits.add(true);
        bits.add(false);
        bits.add(true);

        java.util.Iterator<Boolean> it = bits.iterator();
        assertThat(it.next()).isTrue();
        assertThat(it.next()).isFalse();
        assertThat(it.next()).isTrue();
        assertThat(it.hasNext()).isFalse();
    }

    @Test
    public void listEquality()
    {
        Bits bits = new Bits(0);
        bits.add(true);
        bits.add(false);

        List<Boolean> other = java.util.Arrays.asList(true, false);
        assertThat(bits.equals(other)).isTrue();
        assertThat(other.equals(bits)).isTrue();
    }

    @Test
    public void boundsChecking()
    {
        Bits bits = new Bits(2);

        assertThrows(IndexOutOfBoundsException.class, () -> bits.get(-1));
        assertThrows(IndexOutOfBoundsException.class, () -> bits.get(2));
        assertThrows(IndexOutOfBoundsException.class, () -> bits.set(2, true));
    }

    @Test
    public void reverseBits()
    {
        Bits bits = new Bits(0);
        bits.add(true);
        bits.add(false);
        bits.add(true);
        bits.add(false);

        Bits reversed = bits.reverseBits();
        assertThat(reversed.size()).isEqualTo(4);
        assertThat(reversed.get(0)).isFalse();
        assertThat(reversed.get(1)).isTrue();
        assertThat(reversed.get(2)).isFalse();
        assertThat(reversed.get(3)).isTrue();

        /* The original is unchanged. */
        assertThat(bits.get(0)).isTrue();
        assertThat(bits.get(2)).isTrue();
    }

    @Test
    public void toBytesRoundTrip()
    {
        Bytes bytes = Bytes.of(0xd6, 0xa5);
        assertThat(bytes.toBits().toBytes()).isEqualTo(bytes);
    }

    @Test
    public void fillBitmapToPattern()
    {
        Bits bits = new Bits(4);
        Bits.Cursor cursor = new Bits.Cursor(0);

        bits.fillBitmapTo(cursor, 4, new boolean[] {true, false});

        assertThat(cursor.get()).isEqualTo(4);
        assertThat(bits.get(0)).isTrue();
        assertThat(bits.get(1)).isFalse();
        assertThat(bits.get(2)).isTrue();
        assertThat(bits.get(3)).isFalse();
    }

    @Test
    public void fillBitmapToRespectsTerminateAt()
    {
        Bits bits = new Bits(10);
        Bits.Cursor cursor = new Bits.Cursor(3);

        bits.fillBitmapTo(cursor, 7, new boolean[] {false, true});

        assertThat(cursor.get()).isEqualTo(7);
        assertThat(bits.get(3)).isFalse();
        assertThat(bits.get(4)).isTrue();
        assertThat(bits.get(5)).isFalse();
        assertThat(bits.get(6)).isTrue();
    }

    @Test
    public void fillBitmapToStopAtSize()
    {
        /* The bitmap ends at terminateAt; filling must stop exactly there. */
        Bits bits = new Bits(5);
        Bits.Cursor cursor = new Bits.Cursor(0);

        bits.fillBitmapTo(cursor, 5, new boolean[] {true});

        assertThat(cursor.get()).isEqualTo(5);
        assertThat(bits.get(4)).isTrue();
    }
}
