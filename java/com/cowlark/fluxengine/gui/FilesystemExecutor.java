package com.cowlark.fluxengine.gui;

import static com.google.common.base.Preconditions.checkState;
import static java.util.Objects.requireNonNull;

import com.cowlark.fluxengine.core.EmergencyStopException;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.vfs.Filesystem;
import java.io.UncheckedIOException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.function.Consumer;

/* Runs queued tasks on the thread owned by a mounted FilesystemOperation. Each mount uses a
 * separate executor so pending tasks cannot leak into a later filesystem session. */
class FilesystemExecutor implements Executor
{
    private final BlockingQueue<Runnable> queue = new LinkedBlockingQueue<>();
    private volatile Filesystem filesystem;
    private boolean accepting = true;

    @Override
    public synchronized void execute(Runnable task)
    {
        if (!accepting)
            throw new RejectedExecutionException("Filesystem executor has stopped");
        queue.add(requireNonNull(task));
    }

    Filesystem getFilesystem()
    {
        Filesystem current = filesystem;
        checkState(current != null, "Filesystem is not mounted");
        return current;
    }

    void run(Filesystem filesystem, Consumer<Throwable> onError)
    {
        this.filesystem = requireNonNull(filesystem);
        try
        {
            while (true)
            {
                try
                {
                    queue.take().run();
                } catch (EmergencyStopException e)
                {
                    throw e;
                } catch (UncheckedIOException e)
                {
                    onError.accept(e.getCause());
                } catch (FluxEngineException e)
                {
                    onError.accept(e);
                }
            }
        } catch (InterruptedException e)
        {
            Thread.currentThread().interrupt();
        } finally
        {
            synchronized (this)
            {
                accepting = false;
                this.filesystem = null;
                queue.clear();
            }
        }
    }

    synchronized void stop()
    {
        if (!accepting)
            return;
        accepting = false;
        /* Wake an idle worker and exit through FilesystemOperation's normal cleanup path. */
        queue.add(() -> {
            throw new EmergencyStopException();
        });
    }
}
