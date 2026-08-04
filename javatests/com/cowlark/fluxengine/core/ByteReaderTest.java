package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ByteReaderTest
{
    @Test
    public void reads8And16()
    {
        ByteReader reader = new ByteReader(Bytes.of(0x01, 0x02, 0x03, 0x04, 0x05, 0x06));

        assertThat(reader.read8()).isEqualTo(0x01);
        assertThat(reader.readBe16()).isEqualTo(0x0203);
        assertThat(reader.readLe16()).isEqualTo(0x0504);
        assertThat(reader.read8()).isEqualTo(0x06);
        assertThat(reader.eof()).isTrue();
    }

    @Test
    public void reads24()
    {
        ByteReader reader = new ByteReader(Bytes.of(0x01, 0x02, 0x03, 0x04, 0x05, 0x06));

        assertThat(reader.readBe24()).isEqualTo(0x010203);
        assertThat(reader.readLe24()).isEqualTo(0x060504);
    }

    @Test
    public void reads32()
    {
        ByteReader reader = new ByteReader(Bytes.of(
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08));

        assertThat(reader.readBe32()).isEqualTo(0x01020304);
        assertThat(reader.readLe32()).isEqualTo(0x08070605);
    }

    @Test
    public void reads48()
    {
        ByteReader reader = new ByteReader(Bytes.of(
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
            0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f));

        assertThat(reader.readBe48()).isEqualTo(0x010203040506L);
        assertThat(reader.readLe48()).isEqualTo(0x0f0e0d0c0b0aL);
    }

    @Test
    public void reads64()
    {
        ByteReader reader = new ByteReader(Bytes.of(
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10));

        assertThat(reader.readBe64()).isEqualTo(0x0102030405060708L);
        assertThat(reader.readLe64()).isEqualTo(0x100f0e0d0c0b0a09L);
    }

    @Test
    public void readSlice()
    {
        ByteReader reader = new ByteReader(Bytes.of(1, 2, 3, 4, 5));

        Bytes slice = reader.read(2);
        assertThat(slice.get(0) & 0xff).isEqualTo(1);
        assertThat(slice.get(1) & 0xff).isEqualTo(2);
        assertThat(reader.pos()).isEqualTo(2);
        assertThat(reader.read8()).isEqualTo(3);
    }

    @Test
    public void seekSkipAndEof()
    {
        ByteReader reader = new ByteReader(Bytes.of(1, 2, 3));

        assertThat(reader.pos()).isEqualTo(0);
        assertThat(reader.remaining()).isEqualTo(3);

        assertThat(reader.skip(2).pos()).isEqualTo(2);
        assertThat(reader.eof()).isFalse();
        assertThat(reader.remaining()).isEqualTo(1);

        assertThat(reader.skip(1).eof()).isTrue();
        assertThat(reader.seek(0).pos()).isEqualTo(0);
    }

    @Test
    public void boundsChecking()
    {
        ByteReader reader = new ByteReader(Bytes.of(1, 2, 3));
        reader.seek(3);

        assertThrows(IndexOutOfBoundsException.class, reader::read8);
        assertThrows(IndexOutOfBoundsException.class, () -> reader.seek(2).readBe16());
        assertThrows(IndexOutOfBoundsException.class, () -> reader.seek(0).readBe32());
        assertThrows(IndexOutOfBoundsException.class, () -> reader.seek(0).read(4));
    }
}
