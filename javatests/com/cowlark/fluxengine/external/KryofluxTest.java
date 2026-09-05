package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.Arrays;

@RunWith(JUnit4.class)
public class KryofluxTest
{
    private static void testConvert(Bytes kyrofluxBytes, Bytes expectedFluxmapBytes)
    {
        Fluxmap fluxmap = Kryoflux.readStream(kyrofluxBytes);
        assertThat(fluxmap.rawBytes().toByteArray()).isEqualTo(expectedFluxmapBytes.toByteArray());
    }

    private static Bytes unsignedBytes(int count)
    {
        byte[] data = new byte[count];
        Arrays.fill(data, (byte) 0x3f);
        return new Bytes(data);
    }

    @Test
    public void test_stream_reader()
    {
        testConvert(Bytes.of(), Bytes.of());

        /* Simple one-byte intervals */
        testConvert(Bytes.of(0x20, 0x20, 0x20, 0x20), Bytes.of(0x8f, 0x8f, 0x8f, 0x8f));

        /* One-and-a-half-byte intervals */
        testConvert(
                Bytes.of(0x20, 0x00, 0x10, 0x20, 0x01, 0x10, 0x20),
                Bytes.of(0x8f, 0x87, 0x8f, 0x3f, 0x3f, 0x89, 0x8f));

        /* Two-byte intervals */
        testConvert(
                Bytes.of(0x20, 0x0c, 0x00, 0x10, 0x20, 0x0c, 0x01, 0x10, 0x20),
                Bytes.of(0x8f, 0x87, 0x8f, 0x3f, 0x3f, 0x89, 0x8f));

        /* Overflow */
        testConvert(
                Bytes.of(0x20, 0x0b, 0x10, 0x20),
                Bytes.of(0x8f).concat(unsignedBytes(0x207)).concat(Bytes.of(0xa9, 0x8f)));

        /* Single-byte nop */
        testConvert(Bytes.of(0x20, 0x08, 0x20), Bytes.of(0x8f, 0x8f));

        /* Double-byte nop */
        testConvert(Bytes.of(0x20, 0x09, 0xde, 0x20), Bytes.of(0x8f, 0x8f));

        /* Triple-byte nop */
        testConvert(Bytes.of(0x20, 0x0a, 0xde, 0xad, 0x20), Bytes.of(0x8f, 0x8f));

        /* OOB block */
        testConvert(
                Bytes.of(
                        0x20, /* data before */
                        0x0d, /* OOB */
                        0xaa, /* type byte */
                        0x01, 0x00, /* size of payload, little-endian */
                        0x55, /* payload */
                        0x20  /* data continues */), Bytes.of(0x8f, 0x8f));
    }
}