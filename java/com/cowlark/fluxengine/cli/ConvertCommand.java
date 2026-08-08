package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;
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
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.google.common.collect.ImmutableList;

/**
 * Converts a flux file from one format to another, modelled after
 * src/fe-convert.cc.
 */
public class ConvertCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceFluxFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("flux file to read from")
            .build();
    private ValueFlag<String> destImageFlag = StringFlag.builder()
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

        if ((config.getFluxSink().getType() == FLUXTYPE_DRIVE) ||
                (config.getFluxSource().getType() == FLUXTYPE_DRIVE))
            throw new FluxEngineException("you cannot read or write flux to a hardware device");
        if ((config.getFluxSink().getType() == FLUXTYPE_NOT_SET) ||
                (config.getFluxSource().getType() == FLUXTYPE_NOT_SET))
            throw new FluxEngineException(
                    "you must specify both a source and destination flux filename");

        FluxSource fluxSource = FluxSource.create(config);

        DiskLayout diskLayout = new DiskLayout(config);
        int minCylinder = diskLayout.minPhysicalCylinder;
        int maxCylinder = diskLayout.maxPhysicalCylinder;
        int minHead = diskLayout.minPhysicalHead;
        int maxHead = diskLayout.maxPhysicalHead;
        Logger.log("CONVERT: seen cylinders %d..%d, heads %d..%d",
                minCylinder,
                maxCylinder,
                minHead,
                maxHead);

        FluxSinkFactory fluxSinkFactory = FluxSinkFactory.create(config);
        try (FluxSink fluxSink = fluxSinkFactory.create())
        {
            for (CylinderHead physicalLocation : diskLayout.physicalLocations)
            {
                FluxSourceIterator fi = fluxSource.readFlux(
                        physicalLocation.cylinder(), physicalLocation.head());
                while (fi.hasNext())
                    fluxSink.addFlux(physicalLocation, fi.next());
            }
        }
    }
}
