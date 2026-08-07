package com.cowlark.fluxengine.cli;

import com.google.common.collect.ImmutableList;

public class StubCommand implements Command
{
    private final String name;
    private final String help;

    public StubCommand(String name, String help)
    {
        this.name = name;
        this.help = help;
    }

    @Override
    public String getHelp()
    {
        return help;
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        System.err.printf("fluxengine: '%s' is not implemented yet.\n", name);
    }
}
