package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.subjects.ReplaySubject;
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
    protected ConfigProto configProto = null;
    private boolean started = false;
    private Thread workerThread;
    private boolean disposed = false;

    protected FluxOperation()
    {
    }

    /* Requests that the current operation terminate as soon as possible, at
     * its next testForEmergencyStop checkpoint. */
    public static void requestEmergencyStop()
    {
        Common.setEmergencyStop(true);
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

        return Observable.using(
                () -> this,
                op -> subject.doOnSubscribe(d -> schedule(subject)),
                op -> op.dispose());
    }

    /* Starts the operation on a fresh worker thread, but only once, no matter
     * how many subscribers there are. */
    private void schedule(ReplaySubject<LogMessage> subject)
    {
        boolean start = false;
        synchronized (this)
        {
            if (!started)
            {
                started = true;
                workerThread = new Thread(() -> execute(subject));
                start = true;
            }
        }
        if (start)
            workerThread.start();
    }

    private void execute(ReplaySubject<LogMessage> subject)
    {
        synchronized (lock)
        {
            /* Clear any emergency stop left over from a previous aborted
             * operation. */
            Common.setEmergencyStop(false);

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
     * Safe to call multiple times; only the first call has any effect. That
     * first call joins the worker thread, blocking until it exits, so cleanup
     * only ever happens after the operation has completely finished. */
    public void dispose()
    {
        boolean wasDisposed;
        Thread thread;
        synchronized (this)
        {
            wasDisposed = disposed;
            disposed = true;
            thread = workerThread;
        }
        if (wasDisposed)
            return;

        /* If called from the worker itself (normal completion), joining would
         * deadlock, and there is nothing to wait for anyway. */
        if ((thread != null) && (thread != Thread.currentThread()))
        {
            try
            {
                thread.join();
            } catch (InterruptedException e)
            {
                Thread.currentThread().interrupt();
            }
        }

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
