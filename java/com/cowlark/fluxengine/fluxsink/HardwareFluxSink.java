package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.DriveSettings;
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
    public void addFlux(int track, int side, Fluxmap fluxmap)
    {
        DriveSettings settings = new DriveSettings(config);
        settings.seekPosition = track;
        settings.side = side;
        usbFactory.getConnection().write(settings, fluxmap.rawBytes());
    }

    @Override
    public void close()
    {
        usbFactory.close();
    }
}
