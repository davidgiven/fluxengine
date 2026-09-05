package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

public class VfsGetDiskInfoCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);

    @Override
    public String getHelp()
    {
        return "Read volume metadata off a disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        Filesystem.doWithFilesystem(
                config, fs -> {
                    ImmutableMap<String, String> attributes = fs.getFilesystemMetadata();
                    for (java.util.Map.Entry<String, String> e : attributes.entrySet())
                        System.out.printf("%s=%s%n", e.getKey(), Utils.quoteString(e.getValue()));
                });
    }
}
