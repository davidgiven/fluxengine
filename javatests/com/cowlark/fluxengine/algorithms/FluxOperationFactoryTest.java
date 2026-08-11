package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.testing.TestHelpers;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.disposables.Disposable;
import io.reactivex.rxjava3.schedulers.Schedulers;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxOperationFactoryTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    /* A harness whose run() blocks on a semaphore until the test releases it,
     * then logs a message. */
    private static class Harness extends FluxOperationFactory
    {
        final Semaphore gate = new Semaphore(0);
        final CountDownLatch started = new CountDownLatch(1);
        final CountDownLatch finished = new CountDownLatch(1);
        volatile Thread runThread;

        @Override
        public void run()
        {
            runThread = Thread.currentThread();
            started.countDown();
            try
            {
                gate.acquire();
            } catch (InterruptedException e)
            {
                throw new RuntimeException(e);
            }
            Logger.log(new StringMessage("hello"));
            finished.countDown();
        }
    }

    @Test
    public void multipleSubscribersSeeSameOperation() throws Exception
    {
        Harness harness = new Harness();
        Observable<LogMessage> observable = harness.create();

        List<LogMessage> first = new ArrayList<>();
        List<LogMessage> second = new ArrayList<>();
        CountDownLatch done = new CountDownLatch(2);
        Disposable firstSubscription = observable.subscribe(m -> {
            synchronized (first)
            {
                first.add(m);
            }
        }, t -> {
        }, done::countDown);
        Disposable secondSubscription = observable.subscribe(m -> {
            synchronized (second)
            {
                second.add(m);
            }
        }, t -> {
        }, done::countDown);

        harness.gate.release();

        assertThat(done.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(first).containsExactly(new StringMessage("hello"));
        assertThat(second).containsExactly(new StringMessage("hello"));
    }

    @Test
    public void consecutiveOperationsRunOnDifferentThreads() throws Exception
    {
        Harness first = new Harness();
        first.create().subscribe();
        first.gate.release();

        Harness second = new Harness();
        second.create().subscribe();
        second.gate.release();

        Harness third = new Harness();
        third.create().subscribe();
        third.gate.release();

        assertThat(first.finished.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(second.finished.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(third.finished.await(5, TimeUnit.SECONDS)).isTrue();

        /* Each operation gets its own fresh worker thread, not the test
         * thread, and no two operations share a thread. */
        assertThat(first.runThread).isNotEqualTo(Thread.currentThread());
        assertThat(second.runThread).isNotEqualTo(first.runThread);
        assertThat(third.runThread).isNotEqualTo(first.runThread);
        assertThat(third.runThread).isNotEqualTo(second.runThread);
    }

    @Test
    public void operationsStartedAtSameTimeAreSerialised() throws Exception
    {
        Harness first = new Harness();
        Harness second = new Harness();
        first.create().subscribe();
        second.create().subscribe();

        try
        {
            /* The first operation starts immediately. */
            assertThat(first.started.await(5, TimeUnit.SECONDS)).isTrue();

            /* Only one operation may run at a time: the second must wait
             * until the first has finished. */
            assertThat(second.started.await(100, TimeUnit.MILLISECONDS)).isFalse();

            /* Releasing the first operation lets the second run, on its own
             * thread. */
            first.gate.release();
            assertThat(second.started.await(5, TimeUnit.SECONDS)).isTrue();
            assertThat(second.runThread).isNotEqualTo(first.runThread);

            second.gate.release();
            assertThat(first.finished.await(5, TimeUnit.SECONDS)).isTrue();
            assertThat(second.finished.await(5, TimeUnit.SECONDS)).isTrue();
        } finally
        {
            /* Always release both gates so a failed assertion doesn't leave
             * worker threads blocked. */
            first.gate.release();
            second.gate.release();
        }
    }

    @Test
    public void failingOperationDeliversErrorAndCleansUpLogger() throws Exception
    {
        FluxOperationFactory failing = new FluxOperationFactory()
        {
            @Override
            public void run()
            {
                throw new RuntimeException("boom");
            }
        };

        List<Throwable> errors = new ArrayList<>();
        CountDownLatch done = new CountDownLatch(1);
        Disposable subscription = failing.create().subscribe(m -> {
        }, t -> {
            errors.add(t);
            done.countDown();
        }, done::countDown);

        assertThat(done.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(errors).hasSize(1);

        /* A fresh worker thread must not inherit the failed operation's
         * logger; the default (unset) logger throws. */
        AtomicBoolean loggerThrows = new AtomicBoolean();
        CountDownLatch probed = new CountDownLatch(1);
        Schedulers.newThread().scheduleDirect(() -> {
            try
            {
                Logger.log(new StringMessage("probe"));
            } catch (IllegalStateException e)
            {
                loggerThrows.set(true);
            }
            probed.countDown();
        });

        assertThat(probed.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(loggerThrows.get()).isTrue();
    }
}
