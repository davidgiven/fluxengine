package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.core.FluxEngineException;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public final class UsbFactory
{

    private UsbFactory()
    {
    }

    public static UsbDevice connect(ConfigProto config)
    {
        var device = UsbFinder.selectDevice(config);
        return switch (device.type)
        {
            case GREASEWEAZLE ->
                    new GreaseweazleUsbDevice(device.serialPort, config.getUsb().getGreaseweazle());
            default -> throw new FluxEngineException("unsupported hardware device");

        };
    }

}
