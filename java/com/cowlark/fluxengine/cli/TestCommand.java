package com.cowlark.fluxengine.cli;

import picocli.CommandLine.Command;
import javax.inject.Inject;

@Command(name = "test", description = "Test commands", subcommands = {TestDevicesCommand.class})
public class TestCommand
{
    @Inject
    TestCommand()
    {
    }
}
