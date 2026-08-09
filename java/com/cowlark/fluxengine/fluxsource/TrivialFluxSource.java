package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;

/**
 * A flux source which provides a single flux map per track, ported from
 * lib/fluxsource/fluxsource.h.
 */
public abstract class TrivialFluxSource extends FluxSource
{
    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        return new TrivialFluxSourceIterator(this, parameters);
    }

    public abstract Fluxmap readSingleFlux(FluxReadParameters parameters);
}
