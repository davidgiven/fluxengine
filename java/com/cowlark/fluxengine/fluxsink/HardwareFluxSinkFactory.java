package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.usb.UsbFactory;

/**
 * A factory for hardware flux sinks, ported from lib/fluxsink/hardwarefluxsink.cc.
 */
public class HardwareFluxSinkFactory extends FluxSinkFactory
{
    private final ConfigProto config;
    private final UsbFactory usbFactory;

    public HardwareFluxSinkFactory(ConfigProto config, UsbFactory usbFactory)
    {
        this.config = config;
        this.usbFactory = usbFactory;
    }

    @Override
    public FluxSink create()
    {
        return new HardwareFluxSink(config, usbFactory);
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
