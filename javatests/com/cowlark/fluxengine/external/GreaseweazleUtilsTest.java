package com.cowlark.fluxengine.external;

import static com.cowlark.fluxengine.testing.TestHelpers.buf;
import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.testing.TestHelpers;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GreaseweazleUtilsTest
{
    private static final double CLOCK = 2 * FluxEngine.NS_PER_TICK;

    private static void testConvert(ByteBuf gwBytes, ByteBuf flBytes)
    {
        byte[] expectedFl = ByteBufUtil.getBytes(flBytes);
        byte[] expectedGw = ByteBufUtil.getBytes(gwBytes);

        ByteBuf gwToFl = GreaseweazleUtils.greaseweazleToFluxEngine(
            Unpooled.copiedBuffer(gwBytes), CLOCK);
        ByteBuf flToGw = GreaseweazleUtils.fluxEngineToGreaseweazle(
            Unpooled.copiedBuffer(flBytes), CLOCK);

        assertThat(ByteBufUtil.getBytes(gwToFl)).isEqualTo(expectedFl);
        assertThat(ByteBufUtil.getBytes(flToGw)).isEqualTo(expectedGw);
    }

    private static ByteBuf encode28(int val)
    {
        return buf(1 | (val << 1) & 0xff,
            1 | (val >> 6) & 0xff,
            1 | (val >> 13) & 0xff,
            1 | (val >> 20) & 0xff);
    }

    @Test
    public void conversions()
    {
        /* Simple one-byte intervals. */
        testConvert(
                buf(1, 1, 1, 1, 0),
            buf(0x82, 0x82, 0x82, 0x82));

        /* Larger one-byte intervals. */
        testConvert(
                buf(32, 0),
            buf(0x3f, 0x81));
        testConvert(
                buf(64, 0),
            buf(0x3f, 0x3f, 0x82));
        testConvert(
                buf(128, 0),
            buf(0x3f, 0x3f, 0x3f, 0x3f, 0x84));

        /* Two-byte intervals. */
        testConvert(
                buf(250, 1, 0),
            buf(0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xbb));

        /* Very long intervals. */
        ByteBuf gw = Unpooled.buffer(8);
        gw.writeBytes(buf(255, 2)); /* FLUXOP_SPACE */
        gw.writeBytes(encode28(2048 - 249));
        gw.writeBytes(buf(249, 0));

        ByteBuf fl = Unpooled.buffer(66);
        for (int i = 0; i < 65; i++)
            fl.writeByte(0x3f);
        fl.writeByte(0x81);

        testConvert(gw, fl);
    }
}
