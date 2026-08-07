package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GreaseweazleUtilsTest
{
    private static final double CLOCK = 2 * FluxEngine.NS_PER_TICK;

    private static void testConvert(Bytes gwBytes, Bytes flBytes)
    {
        assertThat(GreaseweazleUtils.greaseweazleToFluxEngine(gwBytes, CLOCK)).isEqualTo(flBytes);
        assertThat(GreaseweazleUtils.fluxEngineToGreaseweazle(flBytes, CLOCK)).isEqualTo(gwBytes);
    }

    private static Bytes encode28(int val)
    {
        return Bytes.of(
                1 | (val << 1) & 0xff,
                1 | (val >> 6) & 0xff,
                1 | (val >> 13) & 0xff,
                1 | (val >> 20) & 0xff);
    }

    @Test
    public void conversions()
    {
        /* Simple one-byte intervals. */
        testConvert(Bytes.of(1, 1, 1, 1, 0), Bytes.of(0x82, 0x82, 0x82, 0x82));

        /* Larger one-byte intervals. */
        testConvert(Bytes.of(32, 0), Bytes.of(0x3f, 0x81));
        testConvert(Bytes.of(64, 0), Bytes.of(0x3f, 0x3f, 0x82));
        testConvert(Bytes.of(128, 0), Bytes.of(0x3f, 0x3f, 0x3f, 0x3f, 0x84));

        /* Two-byte intervals. */
        testConvert(Bytes.of(250, 1, 0), Bytes.of(0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xbb));

        /* Very long intervals. */
        Bytes gw = new Bytes(0);
        new ByteWriter(gw).write8(255)
                .write8(2) /* FLUXOP_SPACE */.write(encode28(2048 - 249))
                .write8(249)
                .write8(0);

        Bytes fl = new Bytes(0);
        ByteWriter bw = new ByteWriter(fl);
        for (int i = 0; i < 65; i++)
            bw.write8(0x3f);
        bw.write8(0x81);

        testConvert(gw, fl);
    }
}
