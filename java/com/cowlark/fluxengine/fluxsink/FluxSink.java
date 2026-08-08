package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * A destination for flux data, ported from lib/fluxsink/fluxsink.h.
 */
public abstract class FluxSink
{
    /* Writes a fluxmap to a track and side. */
    public abstract void addFlux(int track, int side, Fluxmap fluxmap);

    public void addFlux(CylinderHead location, Fluxmap fluxmap)
    {
        addFlux(location.cylinder(), location.head(), fluxmap);
    }
}
