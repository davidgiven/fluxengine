package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.algorithms.ReadOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.google.common.collect.ImmutableList;

/**
 * Read a disk, producing a sector image, modelled after src/fe-read.cc.
 */
public class ReadCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("flux file to read from")
            .build();
    private ValueFlag<String> outputFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--output")
            .setName("-o")
            .setHelpText("destination image to write")
            .build();
    private ValueFlag<String> copyFluxToFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--copy-flux-to")
            .setHelpText("while reading, copy the read flux to this file")
            .build();

    @Override
    public String getHelp()
    {
        return "Reads a disk, producing a sector image.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        if (sourceFlag.isSet())
            builder.withFluxSource(sourceFlag.get());
        if (outputFlag.isSet())
            builder.withImageWriter(outputFlag.get());
        if (copyFluxToFlag.isSet())
            builder.withCopyFluxTo(copyFluxToFlag.get());
        ConfigProto config = builder.build();

        if (config.getDecoder().getCopyFluxTo().getType() == FLUXTYPE_DRIVE)
            throw new FluxEngineException("you cannot copy flux to a hardware device");

        try
        {
            try (ReadOperation operation = new ReadOperation(config))
            {
                operation.run();
            }
        } catch (Exception e)
        {
            throw new RuntimeException(e);
        }
    }
}
