#include <hex/helpers/logger.hpp>
#include <hex/helpers/auto_reset.hpp>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "datastore.h"
#include <thread>
#include <mutex>
#include <deque>
#include <semaphore>

static hex::AutoReset<std::shared_ptr<const DiskFlux>> diskFlux;

static std::deque<std::function<void()>> pendingTasks;
static std::mutex pendingTasksMutex;
static std::thread workerThread;

static void workerThread_cb()
{
    hex::log::debug("worker thread start");

    for (;;)
    {
        std::function<void()> cb;

        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            if (pendingTasks.empty())
            {
                workerThread.detach();
                workerThread = std::move(std::thread());
                hex::log::debug("worker thread shutdown");
                return;
            }

            cb = pendingTasks.front();
            pendingTasks.pop_front();
        }

        hex::log::debug("running worker function {}", (void*)cb.target<void()>());
        cb();
    }
}

std::shared_ptr<const DiskFlux> Datastore::getDiskFlux()
{
    return diskFlux;
}

void Datastore::runOnWorkerThread(std::function<void()> callback)
{
    const std::lock_guard<std::mutex> lock(pendingTasksMutex);
    pendingTasks.push_back(callback);
    if (workerThread.get_id() == std::thread::id())
        workerThread = std::move(std::thread(workerThread_cb));
}
