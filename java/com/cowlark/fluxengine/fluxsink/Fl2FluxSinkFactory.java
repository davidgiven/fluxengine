package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;

/**
 * A factory for FL2 flux sinks, ported from lib/fluxsink/fl2fluxsink.cc.
 */
public class Fl2FluxSinkFactory extends FluxSinkFactory
{
    private final String filename;
    private final ConfigProto config;

    public Fl2FluxSinkFactory(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;
    }

    @Override
    public FluxSink create()
    {
        return new Fl2FluxSink(filename, config);
    }

    @Override
    public String getPath()
    {
        return filename;
    }

    @Override
    public String toString()
    {
        return "fl2(" + filename + ")";
    }
}
