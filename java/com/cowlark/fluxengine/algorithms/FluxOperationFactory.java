package com.cowlark.fluxengine.algorithms;

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
public abstract class FluxOperationFactory implements Runnable
{
    /* Serialises all operations across the whole program: only one may run at
     * a time, because the hardware doesn't cope with concurrent access. */
    private static final Object lock = new Object();

    protected FluxOperationFactory()
    {
    }

    /* Runs the given operation on its own fresh worker thread, forwarding the
     * messages it logs to all subscribers of the returned Observable. */
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

        return subject;
    }

    public abstract void run();
}
