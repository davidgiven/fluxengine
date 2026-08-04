package com.cowlark.fluxengine.external;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;

/**
 * Flux stream conversion helpers, ported from lib/external/greaseweazle.cc.
 */
public final class GreaseweazleUtils
{
    public static final int CMD_GET_INFO = 0;
    public static final int CMD_SEEK = 2;
    public static final int CMD_HEAD = 3;
    public static final int CMD_MOTOR = 6;
    public static final int CMD_READ_FLUX = 7;
    public static final int CMD_WRITE_FLUX = 8;
    public static final int CMD_GET_FLUX_STATUS = 9;
    public static final int CMD_SELECT = 12;
    public static final int CMD_SET_BUS_TYPE = 14;
    public static final int CMD_SET_PIN = 15;
    public static final int CMD_ERASE_FLUX = 17;
    public static final int CMD_SOURCE_BYTES = 18;
    public static final int CMD_SINK_BYTES = 19;

    public static final int ACK_OKAY = 0;
    public static final int ACK_BAD_COMMAND = 1;
    public static final int ACK_NO_INDEX = 2;
    public static final int ACK_NO_TRK0 = 3;
    public static final int ACK_FLUX_OVERFLOW = 4;
    public static final int ACK_FLUX_UNDERFLOW = 5;
    public static final int ACK_WRPROT = 6;
    public static final int ACK_NO_UNIT = 7;
    public static final int ACK_NO_BUS = 8;
    public static final int ACK_BAD_UNIT = 9;
    public static final int ACK_BAD_PIN = 10;
    public static final int ACK_BAD_CYLINDER = 11;

    public static final int GETINFO_FIRMWARE = 0;

    public static final int FLUXOP_INDEX = 1;
    public static final int FLUXOP_SPACE = 2;

    public static final int BAUD_NORMAL = 9600;
    public static final int BAUD_CLEAR_COMMS = 10000;

    private GreaseweazleUtils()
    {
    }

    public static Bytes fluxEngineToGreaseweazle(Bytes fldata, double clock)
    {
        Bytes out = new Bytes(0);
        ByteWriter bw = new ByteWriter(out);
        ByteReader br = new ByteReader(fldata);
        long ticksFl = 0;
        long ticksGw = 0;

        while (!br.eof())
        {
            int b = br.read8();
            ticksFl += b & 0x3f;
            if ((b & F_BIT_PULSE) != 0)
            {
                long newTicksGw = (long) (ticksFl * NS_PER_TICK / clock);
                long delta = newTicksGw - ticksGw;
                if (delta < 250)
                    bw.write8((int) delta);
                else
                {
                    long high = (delta - 250) / 255;
                    if (high < 5)
                    {
                        bw.write8((int) (250 + high));
                        bw.write8((int) (1 + (delta - 250) % 255));
                    }
                    else
                    {
                        bw.write8(255);
                        bw.write8(FLUXOP_SPACE);
                        write28(bw, delta - 249);
                        bw.write8(249);
                    }
                }
                ticksGw = newTicksGw;
            }
        }
        bw.write8(0); /* end of stream */
        return out;
    }

    public static Bytes greaseweazleToFluxEngine(Bytes gwdata, double clock)
    {
        Bytes out = new Bytes(0);
        ByteWriter bw = new ByteWriter(out);
        ByteReader br = new ByteReader(gwdata);
        long ticksGw = 0;
        long lastEventFl = 0;
        long indexGw = -1;

        while (!br.eof())
        {
            int b = br.read8();
            if (b == 0)
                break;

            int event = 0;
            if (b == 255)
            {
                switch (br.read8())
                {
                    case FLUXOP_INDEX:
                        indexGw = ticksGw + read28(br);
                        break;

                    case FLUXOP_SPACE:
                        ticksGw += read28(br);
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
                    long delta = 250 + (b - 250) * 255 + br.read8() - 1;
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
                            bw.write8(0x3f);
                            deltaFl -= 0x3f;
                        }
                        bw.write8((int) (deltaFl | F_BIT_INDEX));
                        lastEventFl = indexFl;
                        indexGw = -1;
                    }
                    else if (indexFl == ticksFl)
                        event |= F_BIT_INDEX;
                }

                long deltaFl = ticksFl - lastEventFl;
                while (deltaFl > 0x3f)
                {
                    bw.write8(0x3f);
                    deltaFl -= 0x3f;
                }
                bw.write8((int) (deltaFl | event));
                lastEventFl = ticksFl;
            }
        }

        return out;
    }

    /* Left-truncates at the first index mark, so the resulting data is aligned
     * at the index. */
    public static Bytes stripPartialRotation(Bytes fldata)
    {
        for (int i = 0; i < fldata.size(); i++)
        {
            if ((fldata.get(i) & F_BIT_INDEX) != 0)
                return fldata.slice(i, fldata.size() - i);
        }
        return fldata;
    }

    private static void write28(ByteWriter out, long val)
    {
        out.write8(1 | (int) (val << 1) & 0xff);
        out.write8(1 | (int) (val >> 6) & 0xff);
        out.write8(1 | (int) (val >> 13) & 0xff);
        out.write8(1 | (int) (val >> 20) & 0xff);
    }

    private static long read28(ByteReader in)
    {
        return (long) ((in.read8() & 0xfe) >> 1) |
               (long) (in.read8() & 0xfe) << 6 |
               (long) (in.read8() & 0xfe) << 13 |
               (long) (in.read8() & 0xfe) << 20;
    }
}
