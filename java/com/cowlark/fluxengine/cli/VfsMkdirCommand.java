package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;

public class VfsMkdirCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> pathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path")
            .setName("-p")
            .setHelpText("directory to create")
            .setDefaultValue("")
            .build();

    @Override
    public String getHelp()
    {
        return "Create a directory on disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        String pathString = pathFlag.get();
        if (pathString.isEmpty())
            throw new FluxEngineException("filename missing");

        Filesystem.doWithFilesystem(
                config, fs -> {
                    fs.createDirectory(VfsPath.of(pathString));
                });
    }
}
