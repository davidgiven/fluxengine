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
        var candidateDevice = UsbFinder.selectDevice(config);
        var device = switch (candidateDevice.type)
        {
            case GREASEWEAZLE -> new GreaseweazleUsbDevice(
                    candidateDevice.serialPort,
                    config.getUsb().getGreaseweazle());
            default -> throw new FluxEngineException("unsupported hardware device");

        };
        device.setDrive(
                config.getDrive().getDrive(),
                config.getDrive().getHighDensity(),
                config.getDrive().getIndexMode().getNumber());
        return device;
    }

}
