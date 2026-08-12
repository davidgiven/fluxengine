package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;

import com.cowlark.fluxengine.algorithms.ReadWriteFluxOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogRenderer;
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

    private class RawwriteOperation extends ReadWriteFluxOperation
    {
        @Override
        public void run()
        {
            rawWrite();
        }
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder().fromFlags(args, flags)
                .withFluxSource(sourceFluxFlag.get())
                .withFluxSink(destFluxFlag.get())
                .build();

        if (config.getFluxSource().getType() == FLUXTYPE_DRIVE)
            throw new FluxEngineException("you can't use rawwrite to read from hardware");

        LogRenderer renderer = LogRenderer.create(System.out);
        new RawwriteOperation().setConfig(config).create().blockingSubscribe(
                renderer::add, e -> {
                    System.err.println("Failed!");
                    e.printStackTrace();
                });
        System.out.println("done.");
    }
}
