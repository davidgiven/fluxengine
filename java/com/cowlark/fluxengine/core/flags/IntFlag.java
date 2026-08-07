package com.cowlark.fluxengine.core.flags;

import lombok.Builder;
import lombok.Singular;
import java.util.List;
import java.util.function.Consumer;

public class IntFlag extends ValueFlag<Integer>
{
    @Builder(setterPrefix = "set")
    private IntFlag(FlagGroup group,
                    @Singular List<String> names,
                    String helpText,
                    int defaultValue,
                    Consumer<Integer> callback)
    {
        super(
                group, names, helpText, defaultValue, callback != null ? callback : unused -> {
                });
    }

    @Override
    public boolean hasArgument()
    {
        return true;
    }

    @Override
    public String defaultValueAsString()
    {
        return Integer.toString(value);
    }

    @Override
    public void set(String value)
    {
        setValue(Integer.parseInt(value));
    }
}
