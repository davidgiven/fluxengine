package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.algorithms.ReadWriteFluxOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Image;
import com.google.common.collect.ImmutableList;

/**
 * Write a sector image to a disk, modelled after src/fe-write.cc.
 */
public class WriteCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceImageFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--input")
            .setName("-i")
            .setHelpText("source image to read from")
            .build();
    private ValueFlag<String> destFluxFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--dest")
            .setName("-d")
            .setHelpText("flux destination to write to")
            .build();
    private boolean verify = true;
    private ActionFlag noVerifyFlag = ActionFlag
            .builder()
            .setGroup(flags)
            .setName("--no-verify")
            .setName("-n")
            .setHelpText("skip verification of write")
            .setVoidCallback(() -> verify = false)
            .build();

    @Override
    public String getHelp()
    {
        return "Writes a sector image to a disk.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder()
                .fromFlags(args, flags)
                .withImageReader(sourceImageFlag.get())
                .withFluxSink(destFluxFlag.get())
                .withFluxSource(destFluxFlag.get()) /* for verification */
                .set("verify_writes", Boolean.toString(verify))
                .build();

        LogRenderer renderer = LogRenderer.create(System.out);
        new WriteOperation().setConfig(config).create().blockingSubscribe(
                renderer::add, e -> {
                    System.err.println("Failed!");
                    e.printStackTrace();
                });
        System.out.println("done.");
    }

    private class WriteOperation extends ReadWriteFluxOperation
    {
        @Override
        public void run()
        {
            Disk disk = new Disk();
            disk.diskLayout = getDiskLayout();
            disk.image = getImageReader().readImage();
            writeDisk(disk);
        }
    }
}
