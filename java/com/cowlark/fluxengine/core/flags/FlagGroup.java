package com.cowlark.fluxengine.core.flags;

import com.cowlark.fluxengine.core.FluxEngineException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;

public class FlagGroup
{
    private final List<FlagGroup> parents;
    private final List<Flag> flags = new ArrayList<>();
    private boolean initialised;

    public FlagGroup()
    {
        parents = List.of();
    }

    public FlagGroup(FlagGroup... parents)
    {
        this.parents = List.of(parents);
    }

    public void addFlag(Flag flag)
    {
        flags.add(flag);
    }

    public void parse(String[] argv)
    {
        List<String> filenames = parseWithFilenames(argv, unused -> false);
        if (!filenames.isEmpty())
            throw new FluxEngineException(
                    "non-option parameter '" + filenames.get(0) + "' seen (try --help)");
    }

    public List<String> parseWithFilenames(String[] argv, Predicate<String> callback)
    {
        if (initialised)
            throw new IllegalStateException("called parse() twice");

        /* Recursively accumulate a list of all flags. */
        Map<String, Flag> flagsByName = new HashMap<>();
        recurse(this, flagsByName);

        List<String> filenames = new ArrayList<>();
        int index = 0;
        while (index < argv.length)
        {
            String thisArg = argv[index];
            String thatArg = (index < argv.length - 1) ? argv[index + 1] : "";

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

                Flag flag = flagsByName.get(key);
                if (flag == null)
                    throw new FluxEngineException("unrecognised flag '" + key + "'; try --help");
                flag.set(value);
                if (useThat && flag.hasArgument())
                    index++;
            }

            index++;
        }

        return filenames;
    }

    public void checkInitialised()
    {
        if (!initialised)
            throw new IllegalStateException("Attempt to access uninitialised flag");
    }

    private void recurse(FlagGroup group, Map<String, Flag> flagsByName)
    {
        if (group.initialised)
            return;

        for (FlagGroup parent : group.parents)
            recurse(parent, flagsByName);

        for (Flag flag : group.flags)
        {
            for (String name : flag.names())
            {
                if (flagsByName.containsKey(name))
                    throw new IllegalStateException("two flags use the name '" + name + "'");
                flagsByName.put(name, flag);
            }
        }

        group.initialised = true;
    }
}
