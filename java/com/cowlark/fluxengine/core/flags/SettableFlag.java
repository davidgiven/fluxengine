package com.cowlark.fluxengine.core.flags;

import java.util.List;
import lombok.Builder;
import lombok.Singular;

public class SettableFlag extends Flag
{
    private boolean value;

    @Builder(setterPrefix = "set")
    private SettableFlag(FlagGroup group, @Singular List<String> names, String helpText)
    {
        super(group, names, helpText);
    }

    public boolean get()
    {
        checkInitialised();
        return value;
    }

    @Override
    public boolean hasArgument()
    {
        return false;
    }

    @Override
    public String defaultValueAsString()
    {
        return "false";
    }

    @Override
    public void set(String value)
    {
        this.value = true;
    }
}
