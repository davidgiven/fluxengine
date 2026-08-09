package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import java.util.Map;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public final class UsbFactory
{

    private UsbFactory()
    {
    }

    private static final Cache<ConfigProto, UsbDevice> cache = CacheBuilder.newBuilder().build();

    /* Connects a USB device, reusing a previously connected device for the
     * same configuration. This is the Java equivalent of the C++ global
     * getUsb(). If a different configuration requires a new device, the
     * previously cached device is evicted and closed. */
    public static synchronized UsbDevice reconnect(ConfigProto config)
    {
        UsbDevice device = cache.getIfPresent(config);
        if (device == null)
        {
            /* Only one device is in use at a time, so any other cached device
             * is being replaced. Close it before opening the new one, since
             * they may share the same serial port. */
            for (Map.Entry<ConfigProto, UsbDevice> entry : cache.asMap().entrySet())
                entry.getValue().close();
            cache.invalidateAll();

            device = connect(config);
            cache.put(config, device);
        }
        return device;
    }

    public static UsbDevice connect(ConfigProto config)
    {
        CandidateDevice candidateDevice = UsbFinder.selectDevice(config);
        UsbDevice device = switch (candidateDevice.type)
        {
            case GREASEWEAZLE -> new GreaseweazleUsbDevice(
                    candidateDevice.serialPort,
                    config.getUsb().getGreaseweazle());
            case APPLESAUCE -> new ApplesauceUsbDevice(
                    candidateDevice.serialPort,
                    config.getUsb().getApplesauce());
            case FLUXENGINE -> new FluxEngineUsbDevice(candidateDevice.device);
            default -> throw new FluxEngineException("unsupported hardware device");

        };

        device.setDrive(
                config.getDrive().getDrive(),
                config.getDrive().getHighDensity(),
                config.getDrive().getIndexMode().getNumber());
        return device;
    }

}
