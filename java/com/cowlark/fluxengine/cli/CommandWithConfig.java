package com.cowlark.fluxengine.cli;

import java.util.List;
import picocli.CommandLine.Command;
import picocli.CommandLine.Unmatched;

/**
 * Base class for commands which accept dotted `--config.flag=value`
 * arguments, which picocli collects as unmatched arguments.
 */
@Command
public abstract class CommandWithConfig
{
    @Unmatched
    private List<String> unmatched;

    protected List<String> unmatchedArguments()
    {
        return unmatched == null ? List.of() : unmatched;
    }

    /* TODO: process the dotted --arguments. */
    protected void processConfigArguments()
    {
    }
}
