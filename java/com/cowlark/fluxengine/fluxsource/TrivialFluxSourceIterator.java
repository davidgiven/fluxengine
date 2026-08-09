package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;

/**
 * Iterator over the single flux map provided by a TrivialFluxSource, ported
 * from lib/fluxsource/fluxsource.cc.
 */
public class TrivialFluxSourceIterator implements FluxSourceIterator
{
    private final TrivialFluxSource fluxSource;
    private final FluxReadParameters parameters;
    private boolean done;

    public TrivialFluxSourceIterator(TrivialFluxSource fluxSource, FluxReadParameters parameters)
    {
        this.fluxSource = fluxSource;
        this.parameters = parameters;
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
        return fluxSource.readSingleFlux(parameters);
    }
}
