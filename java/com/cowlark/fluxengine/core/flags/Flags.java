package com.cowlark.fluxengine.core.flags;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Sets;
import java.util.Set;
import java.util.function.Predicate;

/**
 * Command-line flags system, ported from lib/config/flags.{h,cc}.
 */
public class Flags
{
    public static void parse(ImmutableList<String> argv, FlagGroup... groups)
    {
        parse(argv, ImmutableList.copyOf(groups));
    }

    public static void parse(ImmutableList<String> argv, ImmutableList<FlagGroup> groups)
    {
        ImmutableList<String> filenames = parseWithFilenames(argv, unused -> false, groups);
        if (!filenames.isEmpty())
            throw new FluxEngineException(
                    "non-option parameter '" + filenames.get(0) + "' seen (try --help)");
    }

    public static ImmutableList<String> parseWithFilenames(ImmutableList<String> argv,
                                                           Predicate<String> callback,
                                                           FlagGroup... groups)
    {
        return parseWithFilenames(argv, callback, ImmutableList.copyOf(groups));
    }

    public static ImmutableList<String> parseWithFilenames(ImmutableList<String> argv,
                                                           Predicate<String> callback,
                                                           ImmutableList<FlagGroup> groups)
    {
        if (groups.isEmpty())
            throw new IllegalArgumentException("no flag groups");
        if (groups.get(0).isInitialised())
            throw new IllegalStateException("called parse() twice");

        /* Recursively accumulate a list of all flag names, checking for duplicates. */
        Set<String> names = Sets.newHashSet();
        for (FlagGroup group : groups)
            FlagGroup.initialise(group, names);

        ImmutableList.Builder<String> filenames = ImmutableList.builder();
        int index = 0;
        while (index < argv.size())
        {
            String thisArg = argv.get(index);
            String thatArg = (index < argv.size() - 1) ? argv.get(index + 1) : "";

            String key;
            String value;
            boolean useThat = false;

            if (thisArg.isEmpty())
            {
                /* Ignore this argument. */
            } else if (thisArg.charAt(0) != '-')
            {
                /* This is a filename. */
                if (!callback.test(thisArg))
                    filenames.add(thisArg);
            } else
            {
                if (thisArg.length() > 1 && thisArg.charAt(1) == '-')
                {
                    /* Long option. */
                    int equals = thisArg.lastIndexOf('=');
                    if (equals >= 0)
                    {
                        key = thisArg.substring(0, equals);
                        value = thisArg.substring(equals + 1);
                    } else
                    {
                        key = thisArg;
                        value = thatArg;
                        useThat = true;
                    }
                } else
                {
                    /* Short option. */
                    if (thisArg.length() > 2)
                    {
                        key = thisArg.substring(0, 2);
                        value = thisArg.substring(2);
                    } else
                    {
                        key = thisArg;
                        value = thatArg;
                        useThat = true;
                    }
                }

                Flag flag = null;
                for (FlagGroup group : groups)
                {
                    flag = group.findFlag(key);
                    if (flag != null)
                        break;
                }

                if (flag == null)
                    throw new FluxEngineException("unrecognised flag '" + key + "'; try --help");
                flag.set(value);
                if (useThat && flag.hasArgument())
                    index++;
            }

            index++;
        }

        return filenames.build();
    }

    private Flags()
    {
    }
}
