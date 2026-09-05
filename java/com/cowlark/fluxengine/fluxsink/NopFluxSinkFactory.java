package com.cowlark.fluxengine.fluxsink;

public class NopFluxSinkFactory extends FluxSinkFactory
{
    @Override
    public FluxSink create()
    {
        return new NopFluxSink();
    }
}
