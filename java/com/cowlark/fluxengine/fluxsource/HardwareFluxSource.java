package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.DriveSettings;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;

/**
 * A flux source which reads from real hardware, ported from
 * lib/fluxsource/hardwarefluxsource.cc.
 */
public class HardwareFluxSource extends FluxSource
{
    private final ConfigProto config;
    private final UsbDevice device;

    public HardwareFluxSource(ConfigProto config)
    {
        this(config, UsbFactory.getConnection(config));
    }

    /* Package-private for testing. */
    HardwareFluxSource(ConfigProto config, UsbDevice device)
    {
        this.config = config;
        this.device = device;
    }

    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        return new FluxSourceIterator()
        {
            @Override
            public boolean hasNext()
            {
                return true;
            }

            @Override
            public Fluxmap next()
            {
                DriveSettings settings = new DriveSettings(config);
                settings.seekPosition = parameters.cylinder();
                settings.side = parameters.head();

                Bytes data = device.read(settings, parameters.readTimeNs());
                Fluxmap fluxmap = new Fluxmap();
                fluxmap.appendBytes(data);
                return fluxmap;
            }
        };
    }

    @Override
    public boolean isHardware()
    {
        return true;
    }
}