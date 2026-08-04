package com.cowlark.fluxengine.cli;

import picocli.CommandLine.Command;
import javax.inject.Inject;

@Command(name = "fluxengine", mixinStandardHelpOptions = true, subcommands = {},
        description = "FluxEngine CLI")
public class MainCommand implements Runnable
{
    @Inject
    MainCommand()
    {
    }

    @Override
    public void run()
    {
        System.out.println("run!");
    }
}
