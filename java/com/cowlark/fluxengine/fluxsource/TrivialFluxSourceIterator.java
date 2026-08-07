package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;

/**
 * Iterator over the single flux map provided by a TrivialFluxSource, ported
 * from lib/fluxsource/fluxsource.cc.
 */
public class TrivialFluxSourceIterator implements FluxSourceIterator
{
    private final TrivialFluxSource fluxSource;
    private final int cylinder;
    private final int head;
    private boolean done;

    public TrivialFluxSourceIterator(TrivialFluxSource fluxSource, int cylinder, int head)
    {
        this.fluxSource = fluxSource;
        this.cylinder = cylinder;
        this.head = head;
    }

    @Override
    public boolean hasNext()
    {
        return !done;
    }

    @Override
    public Fluxmap next()
    {
        done = true;
        return fluxSource.readSingleFlux(cylinder, head);
    }
}
