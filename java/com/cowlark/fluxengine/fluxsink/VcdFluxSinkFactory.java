package com.cowlark.fluxengine.fluxsink;

/**
 * A factory for VCD flux sinks, ported from lib/fluxsink/vcdfluxsink.cc.
 */
public class VcdFluxSinkFactory extends FluxSinkFactory
{
    private final String directory;

    public VcdFluxSinkFactory(String directory)
    {
        this.directory = directory;
    }

    @Override
    public FluxSink create()
    {
        return new VcdFluxSink(directory);
    }

    @Override
    public String getPath()
    {
        return directory;
    }

    @Override
    public String toString()
    {
        return "vcd(" + directory + ")";
    }
}
