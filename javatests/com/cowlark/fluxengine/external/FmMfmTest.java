package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FmMfmTest
{
    private static Bits wrapEncodeMfm(Bytes bytes)
    {
        Bits bits = new Bits(16);
        Bits.Cursor cursor = new Bits.Cursor(0);
        boolean[] lastBit = {false};
        FmMfm.encodeMfm(bits, cursor, bytes, lastBit);
        return bits;
    }

    private static Bits wrapEncodeFm(Bytes bytes)
    {
        Bits bits = new Bits(16);
        Bits.Cursor cursor = new Bits.Cursor(0);
        FmMfm.encodeFm(bits, cursor, bytes);
        return bits;
    }

    private static Bits bits(boolean... values)
    {
        Bits bits = new Bits(values.length);
        for (int i = 0; i < values.length; i++)
            bits.setBit(i, values[i]);
        return bits;
    }

    @Test
    public void decode()
    {
        assertThat(FmMfm.decodeFmMfm(bits(
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false))).isEqualTo(Bytes.of(0x00));

        assertThat(FmMfm.decodeFmMfm(bits(
                true,
                true,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                true))).isEqualTo(Bytes.of(0x81));

        assertThat(FmMfm.decodeFmMfm(bits(true, true, true, false))).isEqualTo(Bytes.of(0x80));
    }

    @Test
    public void encodeMfm()
    {
        assertThat(wrapEncodeMfm(Bytes.of(0xa1))).isEqualTo(bits(
                false,
                true,
                false,
                false,
                false,
                true,
                false,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                true));

        assertThat(wrapEncodeMfm(Bytes.of(0xc2))).isEqualTo(bits(
                false,
                true,
                false,
                true,
                false,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                true,
                false,
                false));

        assertThat(wrapEncodeMfm(Bytes.of(0xb0))).isEqualTo(bits(
                false,
                true,
                false,
                false,
                false,
                true,
                false,
                true,
                false,
                false,
                true,
                false,
                true,
                false,
                true,
                false));
    }

    @Test
    public void encodeFm()
    {
        assertThat(wrapEncodeFm(Bytes.of(0x00))).isEqualTo(bits(
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false));

        assertThat(wrapEncodeFm(Bytes.of(0x81))).isEqualTo(bits(
                true,
                true,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                true));
    }
}