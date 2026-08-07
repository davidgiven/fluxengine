package com.cowlark.fluxengine.core.flags;

import lombok.Builder;
import lombok.Singular;
import java.util.List;
import java.util.function.Consumer;

public class DoubleFlag extends ValueFlag<Double>
{
    @Builder(setterPrefix = "set")
    private DoubleFlag(FlagGroup group,
                       @Singular List<String> names,
                       String helpText,
                       Double defaultValue,
                       Consumer<Double> callback)
    {
        super(
                group,
                names,
                helpText,
                defaultValue != null ? defaultValue : 1.0,
                callback != null ? callback : unused -> {
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
        return Double.toString(value);
    }

    @Override
    public void set(String value)
    {
        setValue(Double.parseDouble(value));
    }
}
