package com.cowlark.fluxengine.core.flags;

import lombok.Builder;
import lombok.Singular;
import java.util.List;

public class HexIntFlag extends ValueFlag<Integer>
{
    @Builder(setterPrefix = "set")
    private HexIntFlag(
            FlagGroup group,
            @Singular List<String> names,
            String helpText,
            Integer defaultValue)
    {
        super(group, names, helpText, defaultValue != null ? defaultValue : 0, unused -> {
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
        return String.format("0x%x", value);
    }

    @Override
    public void set(String value)
    {
        setValue(Integer.parseInt(value));
    }
}
