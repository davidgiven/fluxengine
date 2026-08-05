package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.config.ConfigFile.ConfigProto;
import com.google.common.collect.ImmutableList;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigParser
{
    private ConfigProto proto = ConfigProto.getDefaultInstance();

    public ConfigParser()
    {
    }

    public ConfigParser parse(ImmutableList<String> args)
    {
        return this;
    }

    public ConfigParser set(String key, String value){
        return this;
    }

    public ConfigProto build()
    {
        return proto;
    }

}
