package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.algorithms.WriteOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
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
    private boolean erase = false;
    private ActionFlag eraseFlag = ActionFlag.builder()
            .setGroup(flags)
            .setName("--erase")
            .setHelpText("erases the destination")
            .setVoidCallback(this::setErase)
            .build();

    @Override
    public String getHelp()
    {
        return "Writes a flux file to a disk. Warning: you can't use this to copy disks.";
    }

    private void setErase()
    {
        erase = true;
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        if (sourceFluxFlag.isSet())
            builder.withFluxSource(sourceFluxFlag.get());
        String dest = destFluxFlag.isSet() ? destFluxFlag.get() : "drive:0";
        builder.withFluxSink(dest);
        if (erase)
            builder.withFluxSource("erase:");
        ConfigProto config = builder.build();

        if (config.getFluxSource().getType() == FLUXTYPE_DRIVE)
            throw new FluxEngineException("you can't use rawwrite to read from hardware");

        try (WriteOperation operation = new WriteOperation(config))
        {
            operation.writeRawDiskCommand();
        }
    }
}