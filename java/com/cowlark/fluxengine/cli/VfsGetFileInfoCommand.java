package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;

public class VfsGetFileInfoCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> pathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path")
            .setName("-p")
            .setHelpText("path to work on")
            .setDefaultValue("")
            .build();

    @Override
    public String getHelp()
    {
        return "Read file metadata off a disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        String pathString = pathFlag.get();
        VfsPath path = pathString.isEmpty() ? VfsPath.root() : VfsPath.of(pathString);

        Filesystem.doWithFilesystem(
                config, fs -> {
                    Filesystem.Dirent dirent = fs.getDirent(path);
                    for (java.util.Map.Entry<String, String> e : dirent.attributes().entrySet())
                        System.out.printf("%s=%s%n", e.getKey(), Utils.quoteString(e.getValue()));
                });
    }
}
