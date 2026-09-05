package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;

/**
 * A factory for A2R flux sinks, ported from lib/fluxsink/a2rfluxsink.cc.
 */
public class A2RFluxSinkFactory extends FluxSinkFactory
{
    private final String filename;
    private final ConfigProto config;

    public A2RFluxSinkFactory(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;
    }

    @Override
    public FluxSink create()
    {
        return new A2RFluxSink(filename, config);
    }

    @Override
    public String getPath()
    {
        return filename;
    }

    @Override
    public String toString()
    {
        return "a2r(" + filename + ")";
    }
}
