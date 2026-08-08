package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
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
        this(config, UsbFactory.connect(config));
    }

    /* Package-private for testing. */
    HardwareFluxSource(ConfigProto config, UsbDevice device)
    {
        this.config = config;
        this.device = device;
    }

    @Override
    public FluxSourceIterator readFlux(int track, int head)
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
                device.seek(track);

                Bytes data = device.read(
                        head,
                        config.getDrive().getSyncWithIndex(),
                        config.getDrive().getRevolutions() *
                                config.getDrive().getRotationalPeriodMs() * 1e6,
                        config.getDrive().getHardSectorThresholdNs());
                Fluxmap fluxmap = new Fluxmap();
                fluxmap.appendBytes(data);
                return fluxmap;
            }
        };
    }

    @Override
    public void recalibrate()
    {
        device.recalibrate();
    }

    @Override
    public void seek(int track)
    {
        device.seek(track);
    }

    @Override
    public boolean isHardware()
    {
        return true;
    }
}