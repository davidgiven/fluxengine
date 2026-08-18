package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.schedulers.Schedulers;
import io.reactivex.rxjava3.subjects.ReplaySubject;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;

/**
 * Runs an operation once on its own worker thread, storing every log message
 * it emits and replaying them to all subscribers, whether they subscribe
 * before, during or after the operation runs.
 */
public abstract class FluxOperation<T extends FluxOperation<T>> implements Runnable
{
    /* Serialises all operations across the whole program: only one may run at
     * a time, because the hardware doesn't cope with concurrent access. */
    private static final Object lock = new Object();
    private final AtomicBoolean started = new AtomicBoolean(false);
    protected ConfigProto configProto = null;
    private boolean disposed = false;

    protected FluxOperation()
    {
    }

    public ConfigProto getConfig()
    {
        return configProto;
    }

    public FluxOperation<T> setConfig(ConfigProto config)
    {
        this.configProto = config;
        return this;
    }

    /* Returns an Observable which runs the operation on its own fresh worker
     * thread and delivers every message it logs. All messages are stored in a
     * ReplaySubject, so any subscriber sees every message, no matter when it
     * subscribes. The operation starts on the first subscription, and is
     * disposed when the returned Observable terminates. */
    public Observable<LogMessage> create()
    {
        ReplaySubject<LogMessage> subject = ReplaySubject.create();

        return Observable.using(() -> this,
                op -> subject.doOnSubscribe(d -> schedule(subject)),
                op -> op.dispose());
    }

    /* Starts the operation on a fresh worker thread, but only once, no matter
     * how many subscribers there are. */
    private void schedule(ReplaySubject<LogMessage> subject)
    {
        if (started.compareAndSet(false, true))
            Schedulers.newThread().scheduleDirect(() -> execute(subject));
    }

    private void execute(ReplaySubject<LogMessage> subject)
    {
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
