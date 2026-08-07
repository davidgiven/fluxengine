package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * An iterator over no flux at all, ported from lib/fluxsource/fluxsource.h.
 */
public class EmptyFluxSourceIterator implements FluxSourceIterator
{
    @Override
    public boolean hasNext()
    {
        return false;
    }

    @Override
    public Fluxmap next()
    {
        throw new FluxEngineException("no flux to read");
    }
}
