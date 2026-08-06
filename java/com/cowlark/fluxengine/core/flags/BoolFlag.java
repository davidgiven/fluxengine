package com.cowlark.fluxengine.core.flags;

import com.cowlark.fluxengine.core.FluxEngineException;
import java.util.List;
import java.util.function.Consumer;
import lombok.Builder;

public class BoolFlag extends ValueFlag<Boolean>
{
    @Builder(setterPrefix = "set")
    private BoolFlag(FlagGroup group, List<String> names, String helpText,
        boolean defaultValue, Consumer<Boolean> callback)
    {
        super(group, names, helpText, defaultValue,
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
        return value ? "true" : "false";
    }

    @Override
    public void set(String value)
    {
        if (value.equals("true") || value.equals("y"))
            setValue(true);
        else if (value.equals("false") || value.equals("n"))
            setValue(false);
        else
            throw new FluxEngineException(
                "can't parse '" + value + "'; try 'true' or 'false'");
    }
}
