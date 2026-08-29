package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import io.kaitai.struct.KaitaiStream;
import java.io.IOException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class KaitaiByteReaderStreamTest
{
    @Test
    public void readsS1()
    {
        Bytes bytes = new Bytes(new byte[]{1});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readS1()).isEqualTo((byte) 1);
    }

    @Test
    public void readsS2be()
    {
        Bytes bytes = new Bytes(new byte[]{0x02, 0x03});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readS2be()).isEqualTo((short) 0x0203);
    }

@Test
    public void readsS4be()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        // BE: first byte is MSB, so 1*16777216 + 2*65536 + 3*256 + 4 = 16909060
        assertThat(stream.readS4be()).isEqualTo(16909060);
    }

    @Test
    public void readsS8be()
    {
        Bytes bytes = new Bytes(new byte[]{8, 7, 6, 5, 4, 3, 2, 1});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readS8be()).isEqualTo(0x0807060504030201L);
    }

    @Test
    public void readsS2le()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        // LE: first byte is LSB, so 1 + 2*256 = 513
        assertThat(stream.readS2le()).isEqualTo(513);
    }

    @Test
    public void readsS4le()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readS4le()).isEqualTo(0x04030201);
    }

    @Test
    public void readsS8le()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4, 5, 6, 7, 8});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readS8le()).isEqualTo(0x0807060504030201L);
    }

    @Test
    public void readsU1()
    {
        Bytes bytes = new Bytes(new byte[]{(byte) 0x7f});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readU1()).isEqualTo(127);
    }

    @Test
    public void readsU2be()
    {
        Bytes bytes = new Bytes(new byte[]{0x02, 0x01});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readU2be()).isEqualTo(0x0201);
    }

    @Test
    public void readsU4be()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        // BE: first byte is MSB, so 1*16777216 + 2*65536 + 3*256 + 4 = 16909060
        assertThat(stream.readU4be()).isEqualTo(16909060);
    }

    @Test
    public void readsU2le()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        // LE: first byte is LSB, so 1 + 2*256 = 513
        assertThat(stream.readU2le()).isEqualTo(513);
    }

    @Test
    public void readsU4le()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readU4le()).isEqualTo(0x04030201L);
    }

    @Test
    public void readsF4be()
    {
        // 1.0f in IEEE 754: 0x3f800000
        Bytes bytes = new Bytes(new byte[]{(byte) 0x3f, (byte) 0x80, (byte) 0x00, (byte) 0x00});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readF4be()).isEqualTo(1.0f);
    }

    @Test
    public void readsF8be()
    {
        // 1.0d in IEEE 754: 0x3ff0000000000000
        Bytes bytes = new Bytes(new byte[]{
                (byte) 0x3f, (byte) 0xf0, (byte) 0x00, (byte) 0x00,
                (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readF8be()).isEqualTo(1.0d);
    }

    @Test
    public void readsF4le()
    {
        // 1.0f in IEEE 754 LE: 0x0000803f
        Bytes bytes = new Bytes(new byte[]{(byte) 0x00, (byte) 0x00, (byte) 0x80, (byte) 0x3f});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readF4le()).isEqualTo(1.0f);
    }

    @Test
    public void readsF8le()
    {
        // 1.0d in IEEE 754 LE: 0x000000000000f03f
        Bytes bytes = new Bytes(new byte[]{
                (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
                (byte) 0x00, (byte) 0x00, (byte) 0xf0, (byte) 0x3f});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThat(stream.readF8le()).isEqualTo(1.0d);
    }

    @Test
    public void readBytesNotAligned()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4, 5});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        byte[] result = stream.readBytesNotAligned(3);

        assertThat(result).isEqualTo(new byte[]{1, 2, 3});
    }

    @Test
    public void readBytesFull()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4, 5});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        byte[] result = stream.readBytesFull();

        assertThat(result).hasLength(5);
        assertThat(result[0]).isEqualTo(1);
        assertThat(result[1]).isEqualTo(2);
        assertThat(result[2]).isEqualTo(3);
        assertThat(result[3]).isEqualTo(4);
        assertThat(result[4]).isEqualTo(5);
    }

    @Test
    public void readBytesTerm()
    {
        Bytes bytes = new Bytes(new byte[]{'H', 'e', 'l', 'l', 'o', (byte) ',', ' '});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        byte[] result = stream.readBytesTerm((byte) ',', false, false, false);

        assertThat(result).hasLength(5);
        assertThat(result[0]).isEqualTo('H');
        assertThat(result[1]).isEqualTo('e');
        assertThat(result[2]).isEqualTo('l');
        assertThat(result[3]).isEqualTo('l');
        assertThat(result[4]).isEqualTo('o');
    }

    @Test
    public void readBytesTermMulti()
    {
        Bytes bytes = new Bytes(new byte[]{'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', ','});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        byte[] result = stream.readBytesTermMulti(new byte[]{','}, false, false, false);

        assertThat(result).hasLength(12);
        assertThat(result[0]).isEqualTo('H');
        assertThat(result[1]).isEqualTo('e');
        assertThat(result[2]).isEqualTo('l');
        assertThat(result[3]).isEqualTo('l');
        assertThat(result[4]).isEqualTo('o');
        assertThat(result[5]).isEqualTo(',');
        assertThat(result[6]).isEqualTo(' ');
        assertThat(result[7]).isEqualTo('W');
        assertThat(result[8]).isEqualTo('o');
        assertThat(result[9]).isEqualTo('r');
        assertThat(result[10]).isEqualTo('l');
        assertThat(result[11]).isEqualTo('d');
    }

    @Test
    public void readBytesTermEosError()
    {
        Bytes bytes = new Bytes(new byte[]{'H', 'e', 'l', 'l', 'o'});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThrows(RuntimeException.class, () -> stream.readBytesTerm((byte) ',', false, false, true));
    }

    @Test
    public void readBytesTermMultiEosError()
    {
        Bytes bytes = new Bytes(new byte[]{'H', 'e', 'l', 'l', 'o'});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        assertThrows(RuntimeException.class, () -> stream.readBytesTermMulti(new byte[]{','}, false, false, true));
    }

    @Test
    public void substream()
    {
        Bytes bytes = new Bytes(new byte[]{1, 2, 3, 4, 5, 6, 7, 8});
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        KaitaiStream sub = stream.substream(3);

        assertThat(sub.readS1()).isEqualTo((byte) 1);
        assertThat(sub.readS1()).isEqualTo((byte) 2);
        assertThat(sub.readS1()).isEqualTo((byte) 3);
    }

    @Test(expected = UnsupportedOperationException.class)
    public void writeS1Throws()
    {
        Bytes bytes = new Bytes(10);
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        stream.writeS1((byte) 1);
    }

    @Test(expected = UnsupportedOperationException.class)
    public void writeS4beThrows()
    {
        Bytes bytes = new Bytes(10);
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        stream.writeS4be(1);
    }

    @Test(expected = UnsupportedOperationException.class)
    public void writeBytesNotAlignedThrows()
    {
        Bytes bytes = new Bytes(10);
        ByteReader reader = new ByteReader(bytes);
        KaitaiByteReaderStream stream = new KaitaiByteReaderStream(reader);

        stream.writeBytesNotAligned(new byte[]{1, 2, 3});
    }
}