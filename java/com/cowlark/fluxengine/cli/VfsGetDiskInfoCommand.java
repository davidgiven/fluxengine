package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
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

    private static String quote(String s)
    {
        boolean spaces = s.contains(" ");
        if (!spaces && !s.contains("\\") && !s.contains("'") && !s.contains("\""))
            return s;

        StringBuilder ss = new StringBuilder();
        if (spaces)
            ss.append('"');

        for (int i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            if ((c == '\\') || (c == '"') || (c == '!'))
                ss.append('\\');
            ss.append(c);
        }

        if (spaces)
            ss.append('"');

        return ss.toString();
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
                        System.out.printf("%s=%s%n", e.getKey(), quote(e.getValue()));
                });
    }
}
