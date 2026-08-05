package com.cowlark.fluxengine.cli;

import static com.google.common.base.Strings.nullToEmpty;

import com.cowlark.fluxengine.config.Config;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;
import java.util.List;
import picocli.CommandLine.Command;

@Command(name = "devices", description = "List attached USB devices")
public class TestDevicesCommand extends CommandWithConfig implements Runnable
{
    @Override
    public void run()
    {
        UsbFactory usbFactory = new UsbFactory(new Config(ImmutableList.copyOf(unmatchedArguments())));

        List<UsbFactory.CandidateDevice> candidates = usbFactory.findUsbDevices();
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
            for (UsbFactory.CandidateDevice candidate : candidates)
            {
                System.out.println(String.format("%-15s %-30s %s",
                    candidate.type.getDeviceName(),
                    candidate.serial,
                    nullToEmpty(candidate.serialPort)));
            }
        }
    }
}
