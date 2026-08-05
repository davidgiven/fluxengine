package com.cowlark.fluxengine.cli;

import java.util.List;
import com.google.common.collect.ImmutableList;
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

    protected ImmutableList<String> unmatchedArguments()
    {
        return unmatched == null ? ImmutableList.of() : ImmutableList.copyOf(unmatched);
    }
}
