package com.cowlark.fluxengine.fluxsource.FluxSource;

public abstract class FluxSource
{
    public static FluxSource create(FluxSourceProto config)
    {
        return switch (config.getType())
        {
            case FLUXTYPE_ERASE -> new EraseFluxSource(config.getErase());
            default -> null;
        }
    }
}