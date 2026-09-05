package com.cowlark.fluxengine.core.flags;

import java.util.List;
import java.util.function.Consumer;

public abstract class ValueFlag<T> extends Flag
{
    private final Consumer<T> callback;
    protected T value;
    private T defaultValue;
    private boolean isSet;

    protected ValueFlag(
            FlagGroup group,
            List<String> names,
            String helpText,
            T defaultValue,
            Consumer<T> callback)
    {
        super(group, names, helpText);
        this.defaultValue = defaultValue;
        this.value = defaultValue;
        this.callback = callback;
    }

    public T get()
    {
        checkInitialised();
        return value;
    }

    public boolean isSet()
    {
        return isSet;
    }

    public void setDefaultValue(T value)
    {
        defaultValue = value;
        this.value = value;
    }

    protected void setValue(T value)
    {
        this.value = value;
        callback.accept(value);
        isSet = true;
    }
}
