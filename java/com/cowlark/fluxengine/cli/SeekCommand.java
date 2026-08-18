package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.IntFlag;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.usb.DriveSettings;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;

/**
 * Seek to a given track, modelled after src/fe-seek.cc.
 */
public class SeekCommand implements Command
{
    private static FlagGroup flags = new FlagGroup();
    private static IntFlag track = IntFlag.builder()
            .setGroup(flags)
            .setName("--cylinder")
            .setName("-t")
            .setHelpText("track to seek to")
            .build();
    private ValueFlag<String> sourceFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("flux file to read from")
            .build();

    @Override
    public String getHelp()
    {
        return "Moves the disk head.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config =
                new ConfigBuilder().fromFlags(args, flags).withFluxSource(sourceFlag.get()).build();

        if (config.getFluxSource().getType() != FLUXTYPE_DRIVE)
            throw new FluxEngineException("this only makes sense with a real disk drive");

        UsbDevice device = UsbFactory.getConnection(config);
        DriveSettings driveSettings = new DriveSettings(config);
        driveSettings.seekPosition = track.get();
        device.seek(driveSettings);
    }
}
