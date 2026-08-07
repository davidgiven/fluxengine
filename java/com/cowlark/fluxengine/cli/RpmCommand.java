package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;
import java.time.Duration;

/**
 * Measure the disk rotational speed, modelled after src/fe-rpm.cc.
 */
public class RpmCommand implements Command
{
    @Override
    public String getHelp()
    {
        return "Measures the disk rotational speed.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder().fromFlags(args).build();

        if (config.getFluxSource().getType() != FLUXTYPE_DRIVE)
            throw new FluxEngineException("this only makes sense with a real disk drive");

        UsbDevice device = UsbFactory.connect(config);

        Duration period = device.getRotationalPeriod(config.getDrive().getHardSectorCount());
        if (!period.isZero())
            System.out.printf(
                    "Rotational period is %d ms (%.0f rpm)\n",
                    period.toMillis(),
                    60e9 / period.toNanos());
        else
            System.out.println("""
                    No index pulses detected from the disk. Common causes of this are:
                      - no drive is connected
                      - the drive doesn't have an index sensor (e.g. BBC Micro drives)
                      - the disk has no index holes (e.g. reversed flippy disks)
                      - (most common) no disk is inserted in the drive!""");
    }
}
