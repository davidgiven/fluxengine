package com.cowlark.fluxengine.core.flags;

import com.google.common.collect.ImmutableList;
import lombok.AccessLevel;
import lombok.Getter;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class FlagGroup
{
    private final ImmutableList<FlagGroup> parents;
    private final List<Flag> flags = new ArrayList<>();
    @Getter(AccessLevel.PACKAGE) private boolean initialised;

    public FlagGroup()
    {
        parents = ImmutableList.of();
    }

    public FlagGroup(FlagGroup... parents)
    {
        this.parents = ImmutableList.copyOf(parents);
    }

    static void initialise(FlagGroup group, Set<String> names)
    {
        if (group.initialised)
            return;

        for (FlagGroup parent : group.parents)
            initialise(parent, names);

        for (Flag flag : group.flags)
        {
            for (String name : flag.names())
            {
                if (!names.add(name))
                    throw new IllegalStateException("two flags use the name '" + name + "'");
            }
        }

        group.initialised = true;
    }

    public void addFlag(Flag flag)
    {
        flags.add(flag);
    }

    public Flag findFlag(String key)
    {
        for (Flag flag : flags)
        {
            for (String name : flag.names())
            {
                if (name.equals(key))
                    return flag;
            }
        }

        for (FlagGroup parent : parents)
        {
            Flag flag = parent.findFlag(key);
            if (flag != null)
                return flag;
        }

        return null;
    }

    public void checkInitialised()
    {
        if (!initialised)
            throw new IllegalStateException("Attempt to access uninitialised flag");
    }
}
