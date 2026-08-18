package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.UsbFactory;

/**
 * A flux sink which writes to a real floppy drive, ported from
 * lib/fluxsink/hardwarefluxsink.cc.
 */
public class HardwareFluxSink extends FluxSink
{
    private final ConfigProto config;
    private final UsbFactory usbFactory;

    HardwareFluxSink(ConfigProto config, UsbFactory usbFactory)
    {
        this.config = config;
        this.usbFactory = usbFactory;
    }

    @Override
    public void addFlux(int cylinder, int head, Fluxmap fluxmap)
    {
        usbFactory.perform(device -> device.write(cylinder, head, fluxmap.rawBytes()));
    }

    @Override
    public void close()
    {
        usbFactory.close();
    }
}
