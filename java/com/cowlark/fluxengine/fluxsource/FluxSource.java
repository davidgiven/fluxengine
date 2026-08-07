package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.FluxSourceSinkType;
import com.cowlark.fluxengine.core.FluxEngineException;

/**
 * A source of flux data, ported from lib/fluxsource/fluxsource.{h,cc}.
 */
public abstract class FluxSource
{

    public static FluxSource create(FluxSourceProto config)
    {
        switch (config.getType())
        {
            case FLUXTYPE_DRIVE:
                return notImplemented("drive");
            case FLUXTYPE_ERASE:
                return new EraseFluxSource(config.getErase());
            case FLUXTYPE_KRYOFLUX:
                return notImplemented("kryoflux");
            case FLUXTYPE_TEST_PATTERN:
                return notImplemented("test pattern");
            case FLUXTYPE_SCP:
                return notImplemented("scp");
            case FLUXTYPE_A2R:
                return notImplemented("a2r");
            case FLUXTYPE_CWF:
                return notImplemented("cwf");
            case FLUXTYPE_DMK:
                return notImplemented("dmk");
            case FLUXTYPE_FLUX:
                return new Fl2FluxSource(config.getFl2());
            case FLUXTYPE_FLX:
                return notImplemented("flx");
            default:
                return null;
        }
    }

    /* Adjusts the current configuration based on the contents of this flux source. */
    public void adjustConfig(ConfigBuilder configBuilder)
    {
    }

    /* Read flux from a given cylinder and head. */
    public abstract FluxSourceIterator readFlux(int cylinder, int head);

    /* Recalibrates; seeks to cylinder 0 and ensures the head is in the right
     * place. */
    public void recalibrate()
    {
    }

    /* Seeks to a given cylinder (without recalibrating). */
    public void seek(int cylinder)
    {
    }

    /* Is this real hardware? If so, then flux can be read indefinitely (among
     * other things). */
    public boolean isHardware()
    {
        return false;
    }

    private static FluxSource notImplemented(String name)
    {
        throw new FluxEngineException(name + " flux source is not implemented yet");
    }
}
