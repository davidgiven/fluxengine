package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.schedulers.Schedulers;
import io.reactivex.rxjava3.subjects.PublishSubject;
import java.util.function.Consumer;

/**
 * Runs an operation once on its own worker thread, multicasting its log
 * messages to all subscribers via a {@link PublishSubject}.
 */
public abstract class FluxOperation<T extends FluxOperation<T>> implements Runnable
{
    /* Serialises all operations across the whole program: only one may run at
     * a time, because the hardware doesn't cope with concurrent access. */
    private static final Object lock = new Object();

    protected ConfigProto configProto = null;
    private boolean disposed = false;

    protected FluxOperation()
    {
    }

    public FluxOperation<T> setConfig(ConfigProto config)
    {
        this.configProto = config;
        return this;
    }

    public ConfigProto getConfig()
    {
        return configProto;
    }

    /* Runs the given operation on its own fresh worker thread, forwarding the
     * messages it logs to all subscribers of the returned Observable. The
     * factory is disposed when the returned Observable terminates, so that any
     * AutoCloseable resources it holds are released. */
    public Observable<LogMessage> create()
    {
        PublishSubject<LogMessage> subject = PublishSubject.create();

        Schedulers.newThread().scheduleDirect(() -> {
            synchronized (lock)
            {
                Consumer<? super LogMessage> oldLogger = Logger.getLogger();
                Logger.setLogger(subject::onNext);
                try
                {
                    init();
                    run();
                    subject.onComplete();
                } catch (Throwable t)
                {
                    subject.onError(t);
                } finally
                {
                    Logger.setLogger(oldLogger);
                }
            }
        });

        return Observable.using(() -> this, op -> subject, op -> op.dispose());
    }

    /* Disposes the factory, releasing any AutoCloseable resources it holds.
     * Safe to call multiple times; only the first call has any effect. */
    public void dispose()
    {
        boolean wasDisposed;
        synchronized (this)
        {
            wasDisposed = disposed;
            disposed = true;
        }
        if (!wasDisposed)
            onDispose();
    }

    /* Hook for subclasses to close their AutoCloseable resources. Called at
     * most once, by dispose(). */
    protected void onDispose()
    {
    }

    public void init()
    {
    }

    public abstract void run();
}
