package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.testing.TestHelpers;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.disposables.Disposable;
import io.reactivex.rxjava3.schedulers.Schedulers;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

@RunWith(JUnit4.class)
public class FluxOperationTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    /* A harness whose run() blocks on a semaphore until the test releases it,
     * then logs a message. */
    private static class Harness extends FluxOperation<Harness>
    {
        final Semaphore gate = new Semaphore(0);
        final CountDownLatch started = new CountDownLatch(1);
        final CountDownLatch finished = new CountDownLatch(1);
        final AtomicInteger disposeCount = new AtomicInteger();
        final CountDownLatch disposed = new CountDownLatch(1);
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

        @Override
        protected void onDispose()
        {
            disposeCount.incrementAndGet();
            disposed.countDown();
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
        Disposable firstSubscription = observable.subscribe(
                m -> {
                    synchronized (first)
                    {
                        first.add(m);
                    }
                }, t -> {
                }, done::countDown);
        Disposable secondSubscription = observable.subscribe(
                m -> {
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

        /* Both operations race for the lock, so either may acquire it first.
         * Wait for whichever one does, then verify the other is still
         * waiting. */
        long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5);
        while (first.started.getCount() == 1 && second.started.getCount() == 1)
        {
            if (System.nanoTime() >= deadline)
                throw new AssertionError("neither operation started");
            Thread.sleep(1);
        }

        Harness running = first.started.getCount() == 0 ? first : second;
        Harness waiting = running == first ? second : first;

        /* Only one operation may run at a time: the other must wait. */
        assertThat(waiting.started.getCount()).isEqualTo(1);

        /* Releasing the running operation lets the other run, on its own
         * thread. */
        running.gate.release();
        assertThat(waiting.started.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(waiting.runThread).isNotEqualTo(running.runThread);

        waiting.gate.release();
        assertThat(running.finished.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(waiting.finished.await(5, TimeUnit.SECONDS)).isTrue();
    }

    @Test
    public void failingOperationDeliversErrorAndCleansUpLogger() throws Exception
    {
        class TestFluxOperation extends FluxOperation<TestFluxOperation>
        {
            @Override
            public void run()
            {
                throw new RuntimeException("boom");
            }
        }

        TestFluxOperation failing = new TestFluxOperation();
        List<Throwable> errors = new ArrayList<>();
        CountDownLatch done = new CountDownLatch(1);
        Disposable subscription = failing.create().subscribe(
                m -> {
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

    @Test
    public void operationIsDisposedWhenItCompletes() throws Exception
    {
        Harness harness = new Harness();
        harness.create().subscribe();

        harness.gate.release();

        assertThat(harness.finished.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(harness.disposed.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(harness.disposeCount.get()).isEqualTo(1);
    }

    @Test
    public void operationIsDisposedWhenItFails() throws Exception
    {
        Harness harness = new Harness()
        {
            @Override
            public void run()
            {
                throw new RuntimeException("boom");
            }
        };

        Disposable subscription = harness.create().subscribe(
                m -> {
                }, t -> {
                }, () -> {
                });

        assertThat(harness.disposed.await(5, TimeUnit.SECONDS)).isTrue();
        assertThat(harness.disposeCount.get()).isEqualTo(1);
    }

    @Test
    public void disposingSubscriptionDisposesOperation() throws Exception
    {
        Harness harness = new Harness();
        Disposable subscription = harness.create().subscribe();

        try
        {
            assertThat(harness.started.await(5, TimeUnit.SECONDS)).isTrue();

            subscription.dispose();

            assertThat(harness.disposed.await(5, TimeUnit.SECONDS)).isTrue();
            assertThat(harness.disposeCount.get()).isEqualTo(1);
        } finally
        {
            /* Always release the gate so a failed assertion doesn't leave a
             * worker thread blocked. */
            harness.gate.release();
        }
    }

    @Test
    public void disposeIsIdempotent()
    {
        Harness harness = new Harness();

        harness.dispose();
        harness.dispose();

        assertThat(harness.disposeCount.get()).isEqualTo(1);
    }
}
