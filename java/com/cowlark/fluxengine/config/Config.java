package com.cowlark.fluxengine.config;

import com.google.common.collect.ImmutableList;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class Config
{
    private final ImmutableList<String> args;

    public Config(ImmutableList<String> args)
    {
        this.args = args;
        /* TODO: process the unmatched arguments. */
    }
}
