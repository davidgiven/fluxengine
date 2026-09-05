package com.cowlark.fluxengine.core;

import java.util.function.Supplier;

public class SupplierOfAutocloseable<T extends AutoCloseable> implements Supplier<T>, AutoCloseable
{
    private final Supplier<T> delegate;
    public T instance;
    private boolean closed = false;

    public SupplierOfAutocloseable(Supplier<T> delegate)
    {
        if (delegate == null)
            throw new IllegalArgumentException("Delegate supplier cannot be null");
        this.delegate = delegate;
    }

    @Override
    public T get()
    {
        synchronized (this)
        {
            if (closed)
                throw new IllegalStateException("Supplier has already been closed");
            if (instance == null)
                instance = delegate.get();
            return instance;
        }
    }

    @Override
    public void close() throws Exception
    {
        synchronized (this)
        {
            if ((instance != null) && !closed)
                instance.close();
            closed = true;
        }
    }
}