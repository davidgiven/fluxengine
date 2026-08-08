package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;

/**
 * A factory for hardware flux sinks, ported from lib/fluxsink/hardwarefluxsink.cc.
 */
public class HardwareFluxSinkFactory extends FluxSinkFactory
{
    private final ConfigProto config;

    public HardwareFluxSinkFactory(ConfigProto config)
    {
        this.config = config;
    }

    @Override
    public FluxSink create()
    {
        return new HardwareFluxSink(config);
    }

    @Override
    public boolean isHardware()
    {
        return true;
    }

    @Override
    public String toString()
    {
        return "hardware";
    }
}
