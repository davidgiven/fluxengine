package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.TrackFluxProto;

/**
 * Iterator over the flux segments of one track in an FL2 file, ported from
 * lib/fluxsource/fl2fluxsource.cc.
 */
public class Fl2FluxSourceIterator implements FluxSourceIterator
{
    private final TrackFluxProto proto;
    private int count;

    public Fl2FluxSourceIterator(TrackFluxProto proto)
    {
        this.proto = proto;
    }

    @Override
    public boolean hasNext()
    {
        return count < proto.getFluxCount();
    }

    @Override
    public Fluxmap next()
    {
        return new Fluxmap(new Bytes(proto.getFlux(count++).toByteArray()));
    }
}
