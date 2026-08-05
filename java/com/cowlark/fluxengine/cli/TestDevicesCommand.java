package com.cowlark.fluxengine.cli;

import static com.google.common.base.Strings.nullToEmpty;

import com.cowlark.fluxengine.usb.UsbFactory;
import com.cowlark.fluxengine.usb.UsbFactory.CandidateDevice;
import picocli.CommandLine.Command;
import java.util.List;

@Command(name = "devices", description = "List attached USB devices")
public class TestDevicesCommand extends CommandWithConfig implements Runnable
{
    @Override
    public void run()
    {
        List<CandidateDevice> candidates = UsbFactory.findUsbDevices();
        switch (candidates.size())
        {
            case 0:
                System.out.println("Detected no devices.");
                break;

            case 1:
                System.out.println("Detected one device:");
                break;

            default:
                System.out.printf("Detected %d devices:\n", candidates.size());
        }

        if (!candidates.isEmpty())
        {
            System.out.printf(
                    "%-15s %-30s %s\n",
                    "Type",
                    "Serial number",
                    "Port (if any)");
            for (CandidateDevice candidate : candidates)
            {
                System.out.printf(
                        "%-15s %-30s %s\n",
                        candidate.type.getDeviceName(),
                        candidate.serial,
                        nullToEmpty(candidate.serialPort));
            }
        }
    }
}
