package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.usb.UsbFinder;
import picocli.CommandLine.Command;
import javax.inject.Inject;

@Command(name = "devices", description = "List attached USB devices")
public class TestDevicesCommand implements Runnable
{
    private final UsbFinder usbFinder;

    @Inject
    TestDevicesCommand(UsbFinder usbFinder)
    {
        this.usbFinder = usbFinder;
    }

    @Override
    public void run()
    {
        for (UsbFinder.CandidateDevice device : usbFinder.findUsbDevices())
        {
            System.out.println(String.format("%s (0x%08x) serial %s",
                usbFinder.getDeviceName(device.type), device.id, device.serial));
        }
    }
}
