package com.cowlark.fluxengine.core.flags;

import lombok.Builder;
import lombok.Singular;
import java.util.List;
import java.util.function.Consumer;

public class ActionFlag extends Flag
{
    private final Runnable voidCallback;
    private final Consumer<String> valueCallback;
    private final boolean hasArgument;

    @Builder(setterPrefix = "set")
    private ActionFlag(FlagGroup group,
                       @Singular List<String> names,
                       String helpText,
                       Runnable voidCallback,
                       Consumer<String> valueCallback)
    {
        super(group, names, helpText);
        this.voidCallback = voidCallback;
        this.valueCallback = valueCallback;
        hasArgument = valueCallback != null;
    }

    @Override
    public boolean hasArgument()
    {
        return hasArgument;
    }

    @Override
    public String defaultValueAsString()
    {
        return "";
    }

    @Override
    public void set(String value)
    {
        if (hasArgument)
            valueCallback.accept(value);
        else
            voidCallback.run();
    }
}
