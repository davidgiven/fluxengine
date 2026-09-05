package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;

/**
 * A factory for SCP flux sinks, ported from lib/fluxsink/scpfluxsink.cc.
 */
public class ScpFluxSinkFactory extends FluxSinkFactory
{
    private final String filename;
    private final int typeByte;
    private final boolean alignWithIndex;
    private final ConfigProto config;

    public ScpFluxSinkFactory(
            String filename,
            int typeByte,
            boolean alignWithIndex,
            ConfigProto config)
    {
        this.filename = filename;
        this.typeByte = typeByte;
        this.alignWithIndex = alignWithIndex;
        this.config = config;
    }

    @Override
    public FluxSink create()
    {
        return new ScpFluxSink(filename, typeByte, alignWithIndex, config);
    }

    @Override
    public String getPath()
    {
        return filename;
    }

    @Override
    public String toString()
    {
        return "scp(" + filename + ")";
    }
}
