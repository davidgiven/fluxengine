package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.FluxSourceSinkType;
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
        return create(config.getFluxSink());
    }

    public static FluxSinkFactory create(FluxSinkProto config)
    {
        switch (config.getType())
        {
            case FLUXTYPE_DRIVE:
                return notImplemented("hardware");
            case FLUXTYPE_A2R:
                return notImplemented("a2r");
            case FLUXTYPE_AU:
                return notImplemented("au");
            case FLUXTYPE_VCD:
                return notImplemented("vcd");
            case FLUXTYPE_SCP:
                return notImplemented("scp");
            case FLUXTYPE_FLUX:
                return notImplemented("fl2");
            default:
                throw new FluxEngineException("no flux sink specified");
        }
    }

    private static FluxSinkFactory notImplemented(String name)
    {
        throw new FluxEngineException(name + " flux sink is not implemented yet");
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
