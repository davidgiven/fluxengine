package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.FluxSourceSinkType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.usb.UsbFactory;
import java.util.function.Supplier;

/**
 * A source of flux data, ported from lib/fluxsource/fluxsource.{h,cc}.
 */
public abstract class FluxSource implements AutoCloseable
{
    public static FluxSource create(ConfigProto config, Supplier<UsbFactory> usbFactorySupplier)
    {
        if (config.getFluxSource().getType() == FluxSourceSinkType.FLUXTYPE_DRIVE)
            return new HardwareFluxSource(config, usbFactorySupplier.get());
        return create(config.getFluxSource(), usbFactorySupplier);
    }

    public static FluxSource create(FluxSourceProto config, Supplier<UsbFactory> usbFactorySupplier)
    {
        switch (config.getType())
        {
            case FLUXTYPE_DRIVE:
                return notImplemented("hardware");
            case FLUXTYPE_ERASE:
                return new EraseFluxSource(config.getErase());
            case FLUXTYPE_KRYOFLUX:
                return new KryofluxFluxSource(config.getKryoflux());
            case FLUXTYPE_TEST_PATTERN:
                return notImplemented("test pattern");
            case FLUXTYPE_SCP:
                return new ScpFluxSource(config.getScp());
            case FLUXTYPE_A2R:
                return new A2RFluxSource(config.getA2R());
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

    private static FluxSource notImplemented(String name)
    {
        throw new FluxEngineException(name + " flux source is not implemented yet");
    }

    @Override
    public void close() throws Exception
    {
    }

    /* Adjusts the current configuration based on the contents of this flux source. */
    public void adjustConfig(ConfigBuilder configBuilder)
    {
    }

    /* Read flux from a given cylinder and head. */
    public abstract FluxSourceIterator readFlux(FluxReadParameters parameters);

    /* Is this real hardware? If so, then flux can be read indefinitely (among
     * other things). */
    public boolean isHardware()
    {
        return false;
    }
}
