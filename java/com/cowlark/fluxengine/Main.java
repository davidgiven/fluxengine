package com.cowlark.fluxengine;

import picocli.CommandLine;

public class Main
{
    public static void main(String[] args)
    {
        new CommandLine(FluxEngineComponent.create().mainCommand()).execute(args);
    }
}
