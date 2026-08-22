package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Track;
import java.util.Collection;
import java.util.Iterator;

/**
 * An iterator over the flux maps of one location of an in-memory disk,
 * ported from lib/fluxsource/memoryfluxsource.cc.
 */
public class MemoryFluxSourceIterator implements FluxSourceIterator
{
    private final Iterator<Track> iterator;

    public MemoryFluxSourceIterator(Collection<Track> tracks)
    {
        this.iterator = tracks.iterator();
    }

    @Override
    public boolean hasNext()
    {
        return iterator.hasNext();
    }

    @Override
    public Fluxmap next()
    {
        Track track = iterator.next();
        return new Fluxmap(track.fluxmap.rawBytes());
    }
}
