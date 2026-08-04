package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ByteWriterTest
{
    @Test
    public void writes8And16()
    {
        Bytes bytes = new Bytes(0);
        new ByteWriter(bytes)
            .write8(0x01)
            .writeBe16(0x0203)
            .writeLe16(0x0504)
            .write8(0x06);

        assertThat(bytes.toArray()).isEqualTo(new byte[] {1, 2, 3, 4, 5, 6});
    }

    @Test
    public void writes24And32()
    {
        Bytes bytes = new Bytes(0);
        new ByteWriter(bytes)
            .writeBe24(0x010203)
            .writeLe24(0x060504)
            .writeBe32(0x0708090a)
            .writeLe32(0x0e0d0c0b);

        assertThat(bytes.toArray()).isEqualTo(new byte[] {
            1, 2, 3,
            4, 5, 6,
            7, 8, 9, 10,
            11, 12, 13, 14});
    }

    @Test
    public void writes48And64()
    {
        Bytes bytes = new Bytes(0);
        new ByteWriter(bytes)
            .writeBe48(0x010203040506L)
            .writeLe48(0x0c0b0a090807L)
            .writeBe64(0x0102030405060708L)
            .writeLe64(0x100f0e0d0c0b0a09L);

        assertThat(bytes.toArray()).isEqualTo(new byte[] {
            1, 2, 3, 4, 5, 6,
            7, 8, 9, 10, 11, 12,
            1, 2, 3, 4, 5, 6, 7, 8,
            9, 10, 11, 12, 13, 14, 15, 16});
    }

    @Test
    public void writesBytesAndPads()
    {
        Bytes bytes = new Bytes(0);
        new ByteWriter(bytes)
            .write(Bytes.of(1, 2))
            .write(new byte[] {3, 4})
            .pad(2, 0xff)
            .pad(1);

        assertThat(bytes.toArray()).isEqualTo(new byte[] {
            1, 2, 3, 4, (byte) 0xff, (byte) 0xff, 0});
    }

    @Test
    public void growsAndSeeks()
    {
        Bytes bytes = new Bytes(1);
        bytes.set(0, (byte) 0xaa);
        ByteWriter writer = new ByteWriter(bytes);

        writer.seekToEnd().write8(0x01);
        assertThat(bytes.size()).isEqualTo(2);
        assertThat(bytes.get(0) & 0xff).isEqualTo(0xaa);
        assertThat(bytes.get(1) & 0xff).isEqualTo(0x01);

        writer.seek(0).write8(0x02);
        assertThat(bytes.get(0) & 0xff).isEqualTo(0x02);
    }

    @Test
    public void writingToASliceDetachesIt()
    {
        Bytes parent = Bytes.of(1, 2, 3);
        Bytes slice = parent.slice(0, 3);

        new ByteWriter(slice).write8(0xaa);

        assertThat(slice.get(0) & 0xff).isEqualTo(0xaa);
        assertThat(parent.get(0) & 0xff).isEqualTo(1);
    }
}
