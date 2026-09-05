package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.usb.UsbFactory;
import java.util.function.Supplier;

/**
 * Factory for creating flux sinks, ported from lib/fluxsink/fluxsink.h.
 */
public abstract class FluxSinkFactory implements AutoCloseable
{
    public static FluxSinkFactory create(
            ConfigProto config,
            Supplier<UsbFactory> usbFactorySupplier)
    {
        FluxSinkProto fluxSinkProto = config.getFluxSink();
        return switch (fluxSinkProto.getType())
        {
            case FLUXTYPE_DRIVE -> new HardwareFluxSinkFactory(config, usbFactorySupplier.get());
            case FLUXTYPE_A2R ->
                    new A2RFluxSinkFactory(fluxSinkProto.getA2R().getFilename(), config);
            case FLUXTYPE_AU -> new AuFluxSinkFactory(
                    fluxSinkProto.getAu().getDirectory(),
                    fluxSinkProto.getAu().getIndexMarkers());
            case FLUXTYPE_VCD -> new VcdFluxSinkFactory(fluxSinkProto.getVcd().getDirectory());
            case FLUXTYPE_SCP -> new ScpFluxSinkFactory(
                    fluxSinkProto.getScp().getFilename(),
                    fluxSinkProto.getScp().getTypeByte(),
                    fluxSinkProto.getScp().getAlignWithIndex(),
                    config);
            case FLUXTYPE_FLUX ->
                    new Fl2FluxSinkFactory(fluxSinkProto.getFl2().getFilename(), config);
            case FLUXTYPE_NOP -> new NopFluxSinkFactory();
            default -> throw new FluxEngineException("no flux sink specified");
        };
    }

    @Override
    public void close() throws Exception
    {
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
