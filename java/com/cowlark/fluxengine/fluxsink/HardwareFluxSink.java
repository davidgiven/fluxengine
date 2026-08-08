package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;

/**
 * A flux sink which writes to a real floppy drive, ported from
 * lib/fluxsink/hardwarefluxsink.cc.
 */
public class HardwareFluxSink extends FluxSink
{
    private final ConfigProto config;
    private final UsbDevice device;

    public HardwareFluxSink(ConfigProto config)
    {
        this(config, UsbFactory.reconnect(config));
    }

    HardwareFluxSink(ConfigProto config, UsbDevice device)
    {
        this.config = config;
        this.device = device;
    }

    @Override
    public void addFlux(int track, int side, Fluxmap fluxmap)
    {
        device.setDrive(config.getDrive().getDrive(),
                config.getDrive().getHighDensity(),
                config.getDrive().getIndexMode().getNumber());
        device.seek(track);
        device.write(side, fluxmap.rawBytes(), config.getDrive().getHardSectorThresholdNs());
    }

    @Override
    public void close()
    {
        device.close();
    }
}
