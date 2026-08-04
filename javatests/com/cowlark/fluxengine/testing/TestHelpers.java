package com.cowlark.fluxengine.testing;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;

public class TestHelpers
{
    public static ByteBuf buf(int... values)
    {
        byte[] bytes = new byte[values.length];
        for (int i = 0; i < values.length; i++)
            bytes[i] = (byte) values[i];
        return Unpooled.wrappedBuffer(bytes);
    }
}
