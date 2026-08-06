package com.cowlark.fluxengine.core.flags;

import java.util.List;
import java.util.function.Consumer;
import lombok.Builder;
import lombok.Singular;

public class StringFlag extends ValueFlag<String>
{
    @Builder(setterPrefix = "set")
    private StringFlag(FlagGroup group, @Singular List<String> names, String helpText,
        String defaultValue, Consumer<String> callback)
    {
        super(group, names, helpText,
            defaultValue != null ? defaultValue : "",
            callback != null ? callback : unused -> {});
    }

    @Override
    public boolean hasArgument()
    {
        return true;
    }

    @Override
    public String defaultValueAsString()
    {
        return value;
    }

    @Override
    public void set(String value)
    {
        setValue(value);
    }
}
