package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public final class UsbFactory
{

    private UsbFactory()
    {
    }

    public static UsbDevice connect(CandidateDevice device)
    {
        return null;
    }

    public static UsbDevice connect(ConfigProto config)
    {
        return connect(UsbFinder.selectDevice(config));
    }

}
