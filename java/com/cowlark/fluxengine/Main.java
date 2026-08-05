package com.cowlark.fluxengine;

import com.cowlark.fluxengine.cli.CliComponent;
import picocli.CommandLine;

public class Main
{
    public static void main(String[] args)
    {
        FluxEngineComponent component = FluxEngineComponent.create();
        CliComponent cliComponent = component.cliComponentFactory().create();
        CommandLine commandLine =
            new CommandLine(cliComponent.mainCommand(), cliComponent);
        commandLine.execute(args);
    }
}
