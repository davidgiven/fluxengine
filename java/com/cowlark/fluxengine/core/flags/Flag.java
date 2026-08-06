package com.cowlark.fluxengine.core.flags;

import java.util.List;

public abstract class Flag
{
    private final FlagGroup group;
    private final List<String> names;
    private final String helptext;

    protected Flag(FlagGroup group, List<String> names, String helptext)
    {
        this.group = group;
        this.names = List.copyOf(names);
        this.helptext = helptext;
        group.addFlag(this);
    }

    public String name()
    {
        return names.get(0);
    }

    public List<String> names()
    {
        return names;
    }

    public String helptext()
    {
        return helptext;
    }

    public abstract boolean hasArgument();

    public abstract String defaultValueAsString();

    public abstract void set(String value);

    protected void checkInitialised()
    {
        group.checkInitialised();
    }
}
