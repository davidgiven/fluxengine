package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;

/* A factory for A2R3 flux sinks. */
public class A2R3FluxSinkFactory extends FluxSinkFactory
{
    private final String filename;
    private final ConfigProto config;

    public A2R3FluxSinkFactory(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;
    }

    @Override
    public FluxSink create()
    {
        return new A2R3FluxSink(filename, config);
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
