package com.cowlark.fluxengine.external;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;

/**
 * Flux stream conversion helpers, ported from lib/external/greaseweazle.cc.
 */
public final class GreaseweazleUtils
{
    private static final int FLUXOP_INDEX = 1;
    private static final int FLUXOP_SPACE = 2;

    private GreaseweazleUtils()
    {
    }

    public static ByteBuf fluxEngineToGreaseweazle(ByteBuf fldata, double clock)
    {
        ByteBuf out = Unpooled.buffer();
        long ticksFl = 0;
        long ticksGw = 0;

        while (fldata.isReadable())
        {
            int b = fldata.readUnsignedByte();
            ticksFl += b & 0x3f;
            if ((b & F_BIT_PULSE) != 0)
            {
                long newTicksGw = (long) (ticksFl * NS_PER_TICK / clock);
                long delta = newTicksGw - ticksGw;
                if (delta < 250)
                    out.writeByte((int) delta);
                else
                {
                    long high = (delta - 250) / 255;
                    if (high < 5)
                    {
                        out.writeByte((int) (250 + high));
                        out.writeByte((int) (1 + (delta - 250) % 255));
                    }
                    else
                    {
                        out.writeByte(255);
                        out.writeByte(FLUXOP_SPACE);
                        write28(out, delta - 249);
                        out.writeByte(249);
                    }
                }
                ticksGw = newTicksGw;
            }
        }
        out.writeByte(0); /* end of stream */
        return out;
    }

    public static ByteBuf greaseweazleToFluxEngine(ByteBuf gwdata, double clock)
    {
        ByteBuf out = Unpooled.buffer();
        long ticksGw = 0;
        long lastEventFl = 0;
        long indexGw = -1;

        while (gwdata.isReadable())
        {
            int b = gwdata.readUnsignedByte();
            if (b == 0)
                break;

            int event = 0;
            if (b == 255)
            {
                switch (gwdata.readUnsignedByte())
                {
                    case FLUXOP_INDEX:
                        indexGw = ticksGw + read28(gwdata);
                        break;

                    case FLUXOP_SPACE:
                        ticksGw += read28(gwdata);
                        break;

                    default:
                        throw new RuntimeException("bad opcode in Greaseweazle stream");
                }
            }
            else
            {
                if (b < 250)
                    ticksGw += b;
                else
                {
                    long delta = 250 + (b - 250) * 255 + gwdata.readUnsignedByte() - 1;
                    ticksGw += delta;
                }
                event = F_BIT_PULSE;
            }

            if (event != 0)
            {
                long indexFl = Math.round(indexGw * clock / NS_PER_TICK);
                long ticksFl = Math.round(ticksGw * clock / NS_PER_TICK);
                if (indexGw != -1)
                {
                    if (indexFl < ticksFl)
                    {
                        long deltaFl = indexFl - lastEventFl;
                        while (deltaFl > 0x3f)
                        {
                            out.writeByte(0x3f);
                            deltaFl -= 0x3f;
                        }
                        out.writeByte((int) (deltaFl | F_BIT_INDEX));
                        lastEventFl = indexFl;
                        indexGw = -1;
                    }
                    else if (indexFl == ticksFl)
                        event |= F_BIT_INDEX;
                }

                long deltaFl = ticksFl - lastEventFl;
                while (deltaFl > 0x3f)
                {
                    out.writeByte(0x3f);
                    deltaFl -= 0x3f;
                }
                out.writeByte((int) (deltaFl | event));
                lastEventFl = ticksFl;
            }
        }

        return out;
    }

    /* Left-truncates at the first index mark, so the resulting data is aligned
     * at the index. */
    public static ByteBuf stripPartialRotation(ByteBuf fldata)
    {
        for (int i = fldata.readerIndex(); i < fldata.writerIndex(); i++)
        {
            if ((fldata.getByte(i) & F_BIT_INDEX) != 0)
                return fldata.slice(i, fldata.writerIndex() - i);
        }
        return fldata;
    }

    private static void write28(ByteBuf out, long val)
    {
        out.writeByte(1 | (int) (val << 1) & 0xff);
        out.writeByte(1 | (int) (val >> 6) & 0xff);
        out.writeByte(1 | (int) (val >> 13) & 0xff);
        out.writeByte(1 | (int) (val >> 20) & 0xff);
    }

    private static long read28(ByteBuf in)
    {
        return (long) ((in.readUnsignedByte() & 0xfe) >> 1) |
               (long) (in.readUnsignedByte() & 0xfe) << 6 |
               (long) (in.readUnsignedByte() & 0xfe) << 13 |
               (long) (in.readUnsignedByte() & 0xfe) << 20;
    }
}
