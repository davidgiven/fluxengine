#include <hex/helpers/logger.hpp>
#include <hex/helpers/auto_reset.hpp>
#include <hex/api/content_registry/settings.hpp>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "globals.h"
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

        hex::log::debug(
            "running worker function {}", (void*)cb.target<void()>());
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

template <typename T>
static T readSetting(const std::string& leaf, const T defaultValue)
{
    return hex::ContentRegistry::Settings::read<T>(
        FLUXENGINE_CONFIG, "fluxengine.settings." + leaf, defaultValue);
}

static void badConfiguration()
{
    throw ErrorException("internal error: no configuration");
}

static void configure()
{
    try
    {
        auto formatName = readSetting<std::string>("format.selected", "");
        if (formatName.empty())
            badConfiguration();
        auto highDensity = readSetting<bool>("highDensity", true);
        Datastore::runOnWorkerThread(
            [=]
            {
                globalConfig().clear();
                globalConfig().readBaseConfigFile(formatName);
                globalConfig().overrides()->mutable_drive()->set_high_density(
                    highDensity);
            });

        auto device = readSetting<std::string>("device", "fluxfile");
        if (device.empty())
            badConfiguration();
        if (device == "fluxfile")
        {
            auto fluxfile =
                readSetting<std::fs::path>("fluxfile", "unset").string();
            Datastore::runOnWorkerThread(
                [=]
                {
                    globalConfig().setFluxSink(fluxfile);
                    globalConfig().setFluxSource(fluxfile);
                    globalConfig().setVerificationFluxSource(fluxfile);
                });
        }
        else if (device[0] == '#')
        {
            auto serial = device.substr(1);
            Datastore::runOnWorkerThread(
                [=]
                {
                    globalConfig().overrides()->mutable_usb()->set_serial(
                        serial);
                    globalConfig().setFluxSink("drive:0");
                    globalConfig().setFluxSource("drive:0");
                    globalConfig().setVerificationFluxSource("drive:0");
                });
        }
    }
    catch (const ErrorException& e)
    {
        hex::log::error("{}", e.message);
    }
}

void Datastore::beginRead(void)
{
    configure();
}
