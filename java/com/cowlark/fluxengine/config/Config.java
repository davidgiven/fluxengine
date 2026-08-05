package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.wiring.UnmatchedArgs;
import com.google.common.collect.ImmutableList;
import javax.inject.Inject;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class Config
{
    private final ImmutableList<String> args;

    @Inject
    public Config(@UnmatchedArgs ImmutableList<String> args)
    {
        this.args = args;
        /* TODO: process the unmatched arguments. */
    }
}
