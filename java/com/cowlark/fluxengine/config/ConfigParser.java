package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.config.ConfigFile.ConfigProto;
import com.google.common.collect.ImmutableList;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigParser
{
    private ConfigParser()
    {
    }

    public static ConfigProto parse(ImmutableList<String> args)
    {
        return ConfigProto.getDefaultInstance();
    }
}
