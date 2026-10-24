package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.EmergencyStopException;
import com.cowlark.fluxengine.core.FluxEngineException;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FilesystemExecutorTest
{
    private final FilesystemExecutor executor = new FilesystemExecutor();
    private final FakeFilesystem filesystem = new FakeFilesystem();
    private final List<Throwable> errors = new ArrayList<>();
    private final CompletableFuture<Void> finished = new CompletableFuture<>();
    private Thread worker;

    private void startWorker()
    {
        worker = new Thread(() -> {
            try
            {
                executor.run(filesystem, errors::add);
                finished.complete(null);
            } catch (EmergencyStopException e)
            {
                finished.complete(null);
            } catch (Throwable e)
            {
                finished.completeExceptionally(e);
            }
        });
        worker.setDaemon(true);
        worker.start();
    }

    @After
    public void stopWorker() throws Exception
    {
        executor.stop();
        if (worker != null)
        {
            finished.get(5, TimeUnit.SECONDS);
            worker.join(5000);
            assertThat(worker.isAlive()).isFalse();
        }
    }

    @Test
    public void queuedTasksRunInOrderOnTheOperationThread() throws Exception
    {
        List<Integer> order = new ArrayList<>();
        executor.execute(() -> order.add(1));
        executor.execute(() -> order.add(2));
        CompletableFuture<Thread> task = CompletableFuture.supplyAsync(() -> {
            assertThat(executor.getFilesystem()).isSameInstanceAs(filesystem);
            return Thread.currentThread();
        }, executor);
        assertThat(task.isDone()).isFalse();
        startWorker();
        assertThat(task.get(5, TimeUnit.SECONDS)).isSameInstanceAs(worker);
        assertThat(order).containsExactly(1, 2).inOrder();
    }

    @Test
    public void recoverableErrorsAreReportedAndLaterTasksStillRun() throws Exception
    {
        IOException ioFailure = new IOException("disk error");
        FluxEngineException failure = new FluxEngineException("filesystem error");
        executor.execute(() -> {
            throw new UncheckedIOException(ioFailure);
        });
        executor.execute(() -> {
            throw failure;
        });
        CompletableFuture<Void> subsequent = CompletableFuture.runAsync(() -> {}, executor);
        startWorker();
        subsequent.get(5, TimeUnit.SECONDS);
        assertThat(errors).containsExactly(ioFailure, failure).inOrder();
    }

    @Test
    public void stoppingIdleWorkerClearsFilesystemAndRejectsTasks() throws Exception
    {
        assertThrows(IllegalStateException.class, executor::getFilesystem);
        startWorker();
        CompletableFuture.runAsync(() -> {}, executor).get(5, TimeUnit.SECONDS);
        executor.stop();
        finished.get(5, TimeUnit.SECONDS);
        assertThrows(IllegalStateException.class, executor::getFilesystem);
        assertThrows(RejectedExecutionException.class, () -> executor.execute(() -> {}));
        assertThat(errors).isEmpty();
    }

    @Test
    public void interruptingWorkerStopsSession() throws Exception
    {
        startWorker();
        CompletableFuture.runAsync(() -> {}, executor).get(5, TimeUnit.SECONDS);
        worker.interrupt();
        finished.get(5, TimeUnit.SECONDS);
        assertThrows(IllegalStateException.class, executor::getFilesystem);
        assertThrows(RejectedExecutionException.class, () -> executor.execute(() -> {}));
    }
}
