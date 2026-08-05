package com.cowlark.fluxengine.cli;

import picocli.CommandLine.Command;

@Command(name = "fluxengine", mixinStandardHelpOptions = true, subcommands = {TestCommand.class},
        description = "FluxEngine CLI")
public class MainCommand implements Runnable
{
    @Override
    public void run()
    {
        System.out.println("run!");
    }
}
