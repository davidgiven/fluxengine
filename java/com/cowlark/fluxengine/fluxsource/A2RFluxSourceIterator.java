package com.cowlark.fluxengine.fluxsource;

import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import java.util.List;

/**
 * Iterator over the flux revolutions of one track in an A2R file, ported from
 * lib/fluxsource/a2rfluxsource.cc.
 */
class A2RFluxSourceIterator implements FluxSourceIterator
{
    private final List<Bytes> flux;
    private final double index;
    private int count;

    A2RFluxSourceIterator(List<Bytes> flux, double index)
    {
        this.flux = flux;
        this.index = index;
    }

    @Override
    public boolean hasNext()
    {
        return count != flux.size();
    }

    @Override
    public Fluxmap next()
    {
        double index = this.index;
        Bytes asbytes = flux.get(count++);
        ByteReader br = new ByteReader(asbytes);

        Fluxmap fluxmap = new Fluxmap();
        while (!br.eof())
        {
            long aticks = 0;
            for (; ; )
            {
                int i = br.read8();
                aticks += i;
                if (i != 0xff)
                    break;
            }

            double interval = aticks * 125;
            if ((index >= 0) && (index < interval))
            {
                fluxmap.appendInterval((int) index);
                fluxmap.appendIndex();
                interval -= index;
            }
            index -= interval;

            fluxmap.appendInterval((int) (interval / NS_PER_TICK));
            fluxmap.appendPulse();
        }

        return fluxmap;
    }
}