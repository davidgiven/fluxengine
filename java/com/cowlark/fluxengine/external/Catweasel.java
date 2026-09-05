package com.cowlark.fluxengine.external;

import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * Decodes raw Catweasel flux data into a flux map, ported from
 * lib/external/catweasel.cc.
 */
public final class Catweasel
{
    private Catweasel()
    {
    }

    public static Fluxmap decodeCatweaselData(Bytes bytes, double clock)
    {
        Fluxmap fluxmap = new Fluxmap();
        int pending = 0;
        boolean oldIndex = true;
        ByteReader br = new ByteReader(bytes);
        while (!br.eof())
        {
            int b = br.read8();
            boolean index = (b & 0x80) != 0;
            b &= 0x7f;
            if (b == 0x7f)
            {
                pending += 0x7f;
                continue;
            }
            b += pending;
            pending = 0;

            double intervalNs = b * clock;
            fluxmap.appendInterval((int) (intervalNs / NS_PER_TICK));
            fluxmap.appendPulse();

            if (index && !oldIndex)
                fluxmap.appendIndex();
            oldIndex = index;
        }

        return fluxmap;
    }
}
