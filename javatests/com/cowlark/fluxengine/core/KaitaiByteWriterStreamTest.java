package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import java.io.IOException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class KaitaiByteWriterStreamTest
{
    @Test
    public void writesS1()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS1((byte) 0x01);

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{1});
    }

    @Test
    public void writesS2be()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS2be((short) 0x0203);

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{2, 3});
    }

    @Test
    public void writesS4be()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS4be(0x04030201);

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{4, 3, 2, 1});
    }

    @Test
    public void writesS8be()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS8be(0x0807060504030201L);

        assertThat(bytes.toByteArray()).hasLength(8);
    }

    @Test
    public void writesS2le()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS2le((short) 0x0201);

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{1, 2});
    }

    @Test
    public void writesS4le()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS4le(0x04030201);

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{1, 2, 3, 4});
    }

    @Test
    public void writesS8le()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeS8le(0x0807060504030201L);

        assertThat(bytes.toByteArray()).hasLength(8);
    }

    @Test
    public void writesBytesNotAligned()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.writeBytesNotAligned(new byte[]{1, 2, 3});

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{1, 2, 3});
    }

    @Test(expected = UnsupportedOperationException.class)
    public void readS1Throws()
    {
        Bytes bytes = new Bytes(10);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.readS1();
    }

    @Test(expected = UnsupportedOperationException.class)
    public void readS2beThrows()
    {
        Bytes bytes = new Bytes(10);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.readS2be();
    }

    @Test(expected = UnsupportedOperationException.class)
    public void readBytesFullThrows()
    {
        Bytes bytes = new Bytes(10);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.readBytesFull();
    }

    @Test(expected = UnsupportedOperationException.class)
    public void readBytesNotAlignedThrows()
    {
        Bytes bytes = new Bytes(10);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.readBytesNotAligned(5);
    }

    @Test(expected = UnsupportedOperationException.class)
    public void readU1Throws()
    {
        Bytes bytes = new Bytes(10);
        ByteWriter writer = new ByteWriter(bytes);
        KaitaiByteWriterStream stream = new KaitaiByteWriterStream(writer);

        stream.readU1();
    }
}