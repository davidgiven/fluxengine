package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.google.common.collect.ImmutableList;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigBuilder
{
    private ConfigProto proto = ConfigProto.getDefaultInstance();

    public ConfigBuilder()
    {
    }

    public ConfigBuilder fromFlags(ImmutableList<String> args, FlagGroup... group)
    {
        return this;
    }

    public ConfigBuilder set(String key, String value)
    {
        return this;
    }

    public ConfigProto build()
    {
        return proto;
    }

}
