package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;

/**
 * Factory for creating flux sinks, ported from lib/fluxsink/fluxsink.h.
 */
public abstract class FluxSinkFactory
{
    public static FluxSinkFactory create(ConfigProto config)
    {
        if (!config.hasFluxSink())
            throw new FluxEngineException("no flux sink configured");
        return create(config, config.getFluxSink());
    }

    public static FluxSinkFactory create(ConfigProto config, FluxSinkProto sinkConfig)
    {
        switch (sinkConfig.getType())
        {
            case FLUXTYPE_DRIVE:
                return new HardwareFluxSinkFactory(config);
            case FLUXTYPE_A2R:
                return new A2RFluxSinkFactory(sinkConfig.getA2R().getFilename(), config);
            case FLUXTYPE_AU:
                return new AuFluxSinkFactory(
                        sinkConfig.getAu().getDirectory(),
                        sinkConfig.getAu().getIndexMarkers());
            case FLUXTYPE_VCD:
                return new VcdFluxSinkFactory(sinkConfig.getVcd().getDirectory());
            case FLUXTYPE_SCP:
                return new ScpFluxSinkFactory(
                        sinkConfig.getScp().getFilename(),
                        sinkConfig.getScp().getTypeByte(),
                        sinkConfig.getScp().getAlignWithIndex(),
                        config);
            case FLUXTYPE_FLUX:
                return createFl2FluxSinkFactory(sinkConfig.getFl2(), config);
            default:
                throw new FluxEngineException("no flux sink specified");
        }
    }

    public static Fl2FluxSinkFactory createFl2FluxSinkFactory(Fl2FluxSinkProto config,
                                                              ConfigProto fullConfig)
    {
        return new Fl2FluxSinkFactory(config.getFilename(), fullConfig);
    }

    public static Fl2FluxSinkFactory createFl2FluxSinkFactory(String filename,
                                                              ConfigProto fullConfig)
    {
        return new Fl2FluxSinkFactory(filename, fullConfig);
    }

    /* Creates a writer object. */
    public abstract FluxSink create();

    /* Returns whether this is writing to real hardware or not. */
    public boolean isHardware()
    {
        return false;
    }

    /* Returns the path (filename or directory) being written to, if there is
     * one. */
    public String getPath()
    {
        return null;
    }
}
