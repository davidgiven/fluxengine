package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.Flags;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigBuilder
{
    private ConfigProto.Builder proto = ConfigProto.newBuilder();

    public ConfigBuilder()
    {
    }

    public ConfigBuilder fromFlags(ImmutableList<String> args, FlagGroup... group)
    {
        ImmutableList<FlagGroup> allGroups = ImmutableList.<FlagGroup>builder()
                .add(group)
                .add(new ConfigFlagGroup(this))
                .build();
        Flags.parse(args, allGroups);

        return this;
    }

    public ConfigBuilder loadConfigFile(String name)
    {
        String contents;
        try
        {
            contents = Files.readString(Path.of(name));
        }
        catch (IOException e)
        {
            throw new ConfigException("Cannot open '" + name + "': " + e.getMessage());
        }

        try
        {
            TextFormat.merge(contents, proto);
        }
        catch (TextFormat.ParseException e)
        {
            throw new ConfigException("couldn't load external config proto");
        }

        return this;
    }

    public ConfigBuilder showCurrentConfig()
    {
        return this;
    }

    public ConfigBuilder set(String key, String value)
    {
        ProtoPath.set(proto, key, value);
        return this;
    }

    public ConfigProto build()
    {
        return proto.build();
    }

}
