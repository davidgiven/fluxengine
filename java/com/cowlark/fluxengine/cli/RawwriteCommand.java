package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.algorithms.RawWriteOperation;
import com.cowlark.fluxengine.algorithms.WriteOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ConfigProtoOrBuilder;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.google.common.collect.ImmutableList;

/**
 * Write a flux file to a disk, modelled after src/fe-rawwrite.cc.
 */
public class RawwriteCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceFluxFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("source flux file to read from")
            .build();
    private ValueFlag<String> destFluxFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--dest")
            .setName("-d")
            .setHelpText("flux destination to write to")
            .build();

    @Override
    public String getHelp()
    {
        return "Writes a flux file to a disk. Warning: you can't use this to copy disks.";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigProto configProto = new ConfigBuilder().fromFlags(args, flags)
                .withFluxSource(sourceFluxFlag.get())
                .withFluxSink(destFluxFlag.get())
                .build();

        if (configProto.getFluxSource().getType() == FLUXTYPE_DRIVE)
            throw new FluxEngineException("you can't use rawwrite to read from hardware");

        try (RawWriteOperation operation = new RawWriteOperation(configProto))
        {
            operation.rawWrite();
        }
    }
}