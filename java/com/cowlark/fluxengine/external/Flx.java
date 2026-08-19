package com.cowlark.fluxengine.external;

import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * Reads an FLX stream into a flux map, ported from lib/external/flx.cc.
 */
public final class Flx
{
    public static final int FLX_TICK_NS = 40; /* ns per tick */

    public static final int FLX_INDEX = 0x08;
    public static final int FLX_STOP = 0x0d;

    private Flx()
    {
    }

    public static Fluxmap readFlxBytes(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);

        /* Skip header. */

        for (; ; )
        {
            if (br.eof())
                throw new FluxEngineException("malformed FLX stream");
            int b = br.read8();
            if (b == 0)
                break;
        }

        Fluxmap fluxmap = new Fluxmap();
        while (!br.eof())
        {
            int b = br.read8();
            switch (b)
            {
                case FLX_INDEX:
                    fluxmap.appendIndex();
                    break;

                case FLX_STOP:
                    return fluxmap;

                default:
                {
                    if (b < 32)
                        throw new FluxEngineException(String.format("unknown FLX opcode 0x%02x",
                                b));
                    double intervalNs = b * FLX_TICK_NS;
                    fluxmap.appendInterval((int) (intervalNs / NS_PER_TICK));
                    fluxmap.appendPulse();
                    break;
                }
            }
        }

        return fluxmap;
    }
}
