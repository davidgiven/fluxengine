package com.cowlark.fluxengine.cli;

import static com.google.common.base.Strings.nullToEmpty;

import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import java.util.List;

public class DevicesCommand implements Command
{
    private static final FlagGroup EMPTY = new FlagGroup();

    @Override
    public String getHelp()
    {
        return "Displays all detected devices.";
    }

    @Override
    public void run(String[] args)
    {
        List<CandidateDevice> candidates = UsbFinder.findUsbDevices();
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
            System.out.printf("%-15s %-30s %s\n", "Type", "Serial number", "Port (if any)");
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
