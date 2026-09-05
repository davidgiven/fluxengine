package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
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
        FluxSourceProto fluxSourceProto = config.getFluxSource();
        return switch (fluxSourceProto.getType())
        {
            case FLUXTYPE_DRIVE -> new HardwareFluxSource(config, usbFactorySupplier.get());
            case FLUXTYPE_ERASE -> new EraseFluxSource(fluxSourceProto.getErase());
            case FLUXTYPE_KRYOFLUX -> new KryofluxFluxSource(fluxSourceProto.getKryoflux());
            case FLUXTYPE_TEST_PATTERN ->
                    new TestPatternFluxSource(fluxSourceProto.getTestPattern());
            case FLUXTYPE_SCP -> new ScpFluxSource(fluxSourceProto.getScp());
            case FLUXTYPE_A2R -> new A2RFluxSource(fluxSourceProto.getA2R());
            case FLUXTYPE_CWF -> new CwfFluxSource(fluxSourceProto.getCwf());
            case FLUXTYPE_DMK -> new DmkFluxSource(fluxSourceProto.getDmk());
            case FLUXTYPE_FLUX -> new Fl2FluxSource(fluxSourceProto.getFl2());
            case FLUXTYPE_FLX -> new FlxFluxSource(fluxSourceProto.getFlx());
            case FLUXTYPE_NOP -> new NopFluxSource();
            default -> throw new FluxEngineException("no flux source configured");
        };
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
