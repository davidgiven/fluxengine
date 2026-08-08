package com.cowlark.fluxengine.arch.amiga;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AmigaTest
{
    private static final Bytes TEST_DATA = Bytes.of(
            0x52, /* 0101 0010 */
            0xff, /* 1111 1111 */
            0x4a, /* 0100 1010 */
            0x22  /* 0010 0010 */
    );

    private static final Bytes TEST_DATA_INTERLEAVED = Bytes.of(
            0x1f, /* 0001 1111 */
            0x35, /* 0011 0101 */
            0xcf, /* 1100 1111 */
            0x80  /* 1000 0000 */
    );

    @Test
    public void interleave()
    {
        assertThat(Amiga.amigaInterleave(TEST_DATA)).isEqualTo(TEST_DATA_INTERLEAVED);
    }

    @Test
    public void deinterleave()
    {
        assertThat(Amiga.amigaDeinterleave(TEST_DATA_INTERLEAVED)).isEqualTo(TEST_DATA);
    }
}