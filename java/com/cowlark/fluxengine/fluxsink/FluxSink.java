package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * A destination for flux data, ported from lib/fluxsink/fluxsink.h.
 */
public abstract class FluxSink implements AutoCloseable
{
    /* Writes a fluxmap to a track and side. */
    public abstract void addFlux(int track, int side, Fluxmap fluxmap);

    public void addFlux(CylinderHead location, Fluxmap fluxmap)
    {
        addFlux(location.cylinder(), location.head(), fluxmap);
    }

    /* Flushes any buffered data. The C++ writes this in the destructor; Java
     * has no destructor, so this must be called explicitly once all tracks
     * have been written. */
    @Override
    public void close()
    {
    }
}
