package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import java.util.Iterator;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class BitReaderTest
{
    @Test
    public void readsBits()
    {
        Bytes bytes = Bytes.of(0xd6, 0xa0); /* 11010110 10100000 */
        BitReader reader = new BitReader(new ByteReader(bytes));

        boolean[] expected = {
            true, true, false, true, false, true, true, false,
            true, false, true, false, false, false, false, false};
        for (boolean bit : expected)
            assertThat(reader.get()).isEqualTo(bit);
        assertThat(reader.eof()).isTrue();
    }

    @Test
    public void roundTrip()
    {
        Bytes bytes = new Bytes(0);
        new BitWriter(new ByteWriter(bytes))
            .push(0b11010110, 8)
            .push(0b10101100, 8)
            .flush();

        BitReader reader = new BitReader(new ByteReader(bytes));
        boolean[] expected = {
            true, true, false, true, false, true, true, false,
            true, false, true, false, true, true, false, false};
        for (boolean bit : expected)
            assertThat(reader.get()).isEqualTo(bit);
        assertThat(reader.eof()).isTrue();
    }

    @Test
    public void readingPastEndThrows()
    {
        Bytes bytes = Bytes.of(0x80);
        BitReader reader = new BitReader(new ByteReader(bytes));
        for (int i = 0; i < 8; i++)
            reader.get();

        assertThrows(IndexOutOfBoundsException.class, reader::get);
    }

    @Test
    public void iteration()
    {
        Iterator<Boolean> iterator = new BitReader(new ByteReader(Bytes.of(0xd6)));
        boolean[] expected = {
            true, true, false, true, false, true, true, false};
        for (boolean bit : expected)
        {
            assertThat(iterator.hasNext()).isTrue();
            assertThat(iterator.next()).isEqualTo(bit);
        }
        assertThat(iterator.hasNext()).isFalse();
        assertThrows(java.util.NoSuchElementException.class, iterator::next);
    }
}
