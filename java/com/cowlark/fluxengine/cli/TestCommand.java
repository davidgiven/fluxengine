package com.cowlark.fluxengine.cli;

import picocli.CommandLine.Command;

@Command(name = "test", description = "Test commands",
    subcommands = {TestDevicesCommand.class, TestBandwidthCommand.class})
public class TestCommand
{
}
