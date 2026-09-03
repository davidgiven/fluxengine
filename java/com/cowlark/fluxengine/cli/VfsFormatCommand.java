package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.SettableFlag;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.google.common.collect.ImmutableList;

public class VfsFormatCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> volumeNameFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--name")
            .setName("-n")
            .setHelpText("volume name")
            .setDefaultValue("FluxEngine")
            .build();
    private SettableFlag quickFlag = SettableFlag
            .builder()
            .setGroup(flags)
            .setName("--quick")
            .setName("-q")
            .setHelpText("perform quick format (requires the disk to be previously formatted)")
            .build();

    @Override
    public String getHelp()
    {
        return "Format a disk and make a file system on it.";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        Filesystem.doWithFilesystem(
                config, fs -> {
                    fs.create(quickFlag.get(), volumeNameFlag.get());
                    fs.flushChanges();
                });
    }
}
