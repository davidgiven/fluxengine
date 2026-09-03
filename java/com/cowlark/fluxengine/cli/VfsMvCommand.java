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

public class VfsMvCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> oldPathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path1")
            .setHelpText("old filename")
            .setDefaultValue("")
            .build();
    private ValueFlag<String> newPathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path2")
            .setHelpText("new filename")
            .setDefaultValue("")
            .build();

    @Override
    public String getHelp()
    {
        return "Rename a file on a disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        String oldString = oldPathFlag.get();
        if (oldString.isEmpty())
            throw new FluxEngineException("old filename missing");

        String newString = newPathFlag.get();
        if (newString.isEmpty())
            throw new FluxEngineException("new filename missing");

        Filesystem.doWithFilesystem(
                config, fs -> {
                    fs.moveFile(VfsPath.of(oldString), VfsPath.of(newString));
                });
    }
}
