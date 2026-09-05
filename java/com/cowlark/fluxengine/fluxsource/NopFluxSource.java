package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.data.Fluxmap;

public class NopFluxSource extends FluxSource
{
    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        return new FluxSourceIterator()
        {
            @Override
            public boolean hasNext()
            {
                return false;
            }

            @Override
            public Fluxmap next()
            {
                return null;
            }
        };
    }
}
