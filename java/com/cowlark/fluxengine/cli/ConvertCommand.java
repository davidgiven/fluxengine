package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_NOT_SET;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;
import java.util.function.Supplier;

/**
 * Converts a flux file from one format to another, modelled after
 * src/fe-convert.cc.
 */
public class ConvertCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceFluxFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("flux file to read from")
            .build();
    private ValueFlag<String> destImageFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--dest")
            .setName("-d")
            .setHelpText("flux file to write to")
            .build();

    @Override
    public String getHelp()
    {
        return "Converts a flux file from one format to another.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        if (sourceFluxFlag.isSet())
            builder.withFluxSource(sourceFluxFlag.get());
        if (destImageFlag.isSet())
            builder.withFluxSink(destImageFlag.get());
        ConfigProto config = builder.build();

        if ((config.getFluxSink().getType() == FLUXTYPE_NOT_SET) ||
                (config.getFluxSource().getType() == FLUXTYPE_NOT_SET))
            throw new FluxEngineException(
                    "you must specify both a source and destination flux filename");

        Supplier<UsbFactory> notWithHardware = () -> {
            throw new FluxEngineException("you can't use convert with a hardware device");
        };

        FluxSource fluxSource = FluxSource.create(config, notWithHardware);

        DiskLayout diskLayout = new DiskLayout(config);
        int minCylinder = diskLayout.minPhysicalCylinder;
        int maxCylinder = diskLayout.maxPhysicalCylinder;
        int minHead = diskLayout.minPhysicalHead;
        int maxHead = diskLayout.maxPhysicalHead;
        Logger.logf(
                "CONVERT: seen cylinders %d..%d, heads %d..%d",
                minCylinder,
                maxCylinder,
                minHead,
                maxHead);

        FluxSinkFactory fluxSinkFactory = FluxSinkFactory.create(config, notWithHardware);
        try (FluxSink fluxSink = fluxSinkFactory.create())
        {
            for (CylinderHead physicalLocation : diskLayout.physicalLocations)
            {
                FluxSourceIterator fi = fluxSource.readFlux(FluxReadParameters
                        .builder()
                        .setCylinder(physicalLocation.cylinder())
                        .setHead(physicalLocation.head())
                        .build());
                while (fi.hasNext())
                    fluxSink.addFlux(physicalLocation, fi.next());
            }
        }
    }
}
