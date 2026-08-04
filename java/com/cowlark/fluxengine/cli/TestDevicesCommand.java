package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.usb.UsbFinder;
import java.util.List;
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
        List<UsbFinder.CandidateDevice> candidates = usbFinder.findUsbDevices();
        switch (candidates.size())
        {
            case 0:
                System.out.println("Detected no devices.");
                break;

            case 1:
                System.out.println("Detected one device:");
                break;

            default:
                System.out.println(String.format("Detected %d devices:", candidates.size()));
        }

        if (!candidates.isEmpty())
        {
            System.out.println(String.format("%-15s %-30s %s",
                "Type", "Serial number", "Port (if any)"));
            for (UsbFinder.CandidateDevice candidate : candidates)
            {
                System.out.println(String.format("%-15s %-30s %s",
                    candidate.type.getDeviceName(),
                    candidate.serial,
                    candidate.serialPort == null ? "" : candidate.serialPort));
            }
        }
    }
}
