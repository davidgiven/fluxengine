package com.cowlark.fluxengine.cli;

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
    public void run(String[] args)
    {
        System.err.printf("fluxengine: '%s' is not implemented yet.\n", name);
    }
}
