package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.UsbFactory;

/**
 * A flux source which reads from real hardware, ported from
 * lib/fluxsource/hardwarefluxsource.cc.
 */
public class HardwareFluxSource extends FluxSource
{
    private final ConfigProto config;
    private UsbFactory usbFactory;

    /* Package-private for testing. */
    HardwareFluxSource(ConfigProto config, UsbFactory usbFactory)
    {
        this.config = config;
        this.usbFactory = usbFactory;
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
                usbFactory.getConnection().seek(parameters.cylinder());
                Bytes data = usbFactory.getConnection().read(
                        parameters.cylinder(), parameters.head(), parameters.readTimeNs());
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