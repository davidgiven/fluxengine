package com.cowlark.fluxengine.fluxsink;

/**
 * A factory for Sun .au flux sinks, ported from lib/fluxsink/aufluxsink.cc.
 */
public class AuFluxSinkFactory extends FluxSinkFactory
{
    private final String directory;
    private final boolean indexMarkers;

    public AuFluxSinkFactory(String directory, boolean indexMarkers)
    {
        this.directory = directory;
        this.indexMarkers = indexMarkers;
    }

    @Override
    public FluxSink create()
    {
        return new AuFluxSink(directory, indexMarkers);
    }

    @Override
    public String getPath()
    {
        return directory;
    }

    @Override
    public String toString()
    {
        return "au(" + directory + ")";
    }
}
