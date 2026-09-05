package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;

/**
 * Iterator over the flux maps of one track, ported from
 * lib/fluxsource/fluxsource.h.
 */
public interface FluxSourceIterator
{
    boolean hasNext();

    Fluxmap next();
}
