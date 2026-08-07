package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class BitWriterTest
{
    @Test
    public void writesWholeByte()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter bw = new ByteWriter(bytes);
        new BitWriter(bw).push(0b11010110, 8).flush();

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{(byte) 0xd6});
    }

    @Test
    public void packsAcrossBytes()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter bw = new ByteWriter(bytes);
        new BitWriter(bw).push(0b11010110, 8).push(0b101, 3).flush();

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{(byte) 0xd6, 0x05});
    }

    @Test
    public void flushesPartialByte()
    {
        Bytes bytes = new Bytes(0);
        ByteWriter bw = new ByteWriter(bytes);
        new BitWriter(bw).push(0b101, 3).flush();

        assertThat(bytes.toByteArray()).isEqualTo(new byte[]{0x05});
    }
}
