#include <hex/helpers/logger.hpp>
#include <hex/helpers/auto_reset.hpp>
#include <hex/api/imhex_api/hex_editor.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/task_manager.hpp>
#include <toasts/toast_notification.hpp>
#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "lib/data/image.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/decoders/decoders.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/usb/usbfinder.h"
#include "lib/vfs/vfs.h"
#include "arch/arch.h"
#include "globals.h"
#include "datastore.h"
#include "utils.h"
#include <thread>
#include <mutex>
#include <deque>
#include <semaphore>

using hex::operator""_lang;

static std::shared_ptr<const DiskFlux> diskFlux;

static std::deque<std::function<void()>> pendingTasks;
static std::mutex pendingTasksMutex;
static std::thread workerThread;
static std::atomic<bool> busy;

static bool configurationValid;
static bool formattingSupported;
static std::map<std::string, Datastore::Device> devices;
static std::shared_ptr<const DiskLayout> diskLayout;
static std::map<CylinderHead, std::shared_ptr<const TrackInfo>>
    physicalCylinderLayouts;
static std::map<CylinderHeadSector, std::shared_ptr<const Sector>>
    sectorByPhysicalLocation;
static std::map<LogicalLocation, std::shared_ptr<const Sector>>
    sectorByLogicalLocation;
static Layout::LayoutBounds diskPhysicalBounds;
static Layout::LayoutBounds diskLogicalBounds;

static void workerThread_cb()
{
    hex::log::debug("worker thread start");

    auto stopWorkerThread = []
    {
        workerThread.detach();
        workerThread = std::move(std::thread());
        hex::log::debug("worker thread shutdown");
        emergencyStop = busy = false;
    };

    for (;;)
    {
        std::function<void()> cb;

        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            if (pendingTasks.empty())
            {
                stopWorkerThread();
                return;
            }

            cb = pendingTasks.front();
            pendingTasks.pop_front();
        }

        hex::log::debug("running worker function");

        try
        {
            cb();
        }
        catch (const ErrorException& e)
        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            hex::log::debug("worker exception: {}", e.message);
            hex::TaskManager::doLater(
                [=]
                {
                    hex::ui::ToastError::open(
                        fmt::format("FluxEngine error: {}", e.message));
                });
            stopWorkerThread();
            return;
        }
        catch (const EmergencyStopException& e)
        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            hex::log::debug("worker emergency stop");
            hex::TaskManager::doLater(
                [=]
                {
                    hex::ui::ToastError::open("FluxEngine operation cancelled");
                });
            stopWorkerThread();
            return;
        }
        catch (...)
        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            hex::log::debug("worker thread threw an unknown exception");
            hex::TaskManager::doLater(
                [=]
                {
                    hex::ui::ToastError::open(
                        "FluxEngine worker thread died mysteriously");
                });
            stopWorkerThread();
            return;
        }
    }
}

void Datastore::runOnWorkerThread(std::function<void()> callback)
{
    const std::lock_guard<std::mutex> lock(pendingTasksMutex);
    pendingTasks.push_back(callback);
    if (workerThread.get_id() == std::thread::id())
        workerThread = std::move(std::thread(workerThread_cb));
}

void Datastore::init()
{
    runOnWorkerThread(
        []
        {
            Logger::setLogger(
                [](const AnyLogMessage& message)
                {
                    hex::TaskManager::doLater(
                        [=]
                        {
                            Datastore::onLogMessage(message);
                        });
                });
        });

    probeDevices();

    Events::SeekToSectorViaPhysicalLocation::subscribe(
        [](CylinderHeadSector physicalLocation)
        {
            auto it = sectorByPhysicalLocation.find(physicalLocation);
            if (diskFlux && diskFlux->image &&
                (it != sectorByPhysicalLocation.end()))
            {
                auto sector = it->second;
                unsigned offset =
                    diskFlux->layout->sectorOffsetByLogicalLocation.at(
                        {sector->logicalCylinder,
                            sector->logicalHead,
                            sector->logicalSector});

                hex::ImHexApi::HexEditor::setSelection(
                    hex::Region{offset, sector->trackLayout->sectorSize});
            }
        });

    Events::SeekToTrackViaPhysicalLocation::subscribe(
        [](CylinderHead physicalLocation)
        {
            if (!diskFlux)
                return;
            auto ptlo =
                findOptionally(diskFlux->layout->layoutByPhysicalLocation,
                    {physicalLocation.cylinder, physicalLocation.head});
            if (!ptlo.has_value())
                return;
            auto ptl = *ptlo;
            auto ltl = ptl->logicalTrackLayout;
            unsigned firstSectorId = ltl->filesystemSectorOrder.front();
            unsigned lastSectorId = ltl->filesystemSectorOrder.back();

            unsigned startOffset =
                diskFlux->layout->sectorOffsetByLogicalLocation.at(
                    {ltl->logicalCylinder, ltl->logicalHead, firstSectorId});
            unsigned endOffset =
                diskFlux->layout->sectorOffsetByLogicalLocation.at(
                    {ltl->logicalCylinder, ltl->logicalHead, lastSectorId}) +
                ltl->sectorSize;

            hex::ImHexApi::HexEditor::setSelection(
                hex::Region{startOffset, endOffset - startOffset});
        });
    Datastore::rebuildConfiguration();
}

bool Datastore::isBusy()
{
    return busy;
}

bool Datastore::isConfigurationValid()
{
    return configurationValid;
}

bool Datastore::canFormat()
{
    return formattingSupported;
}

const std::map<std::string, Datastore::Device>& Datastore::getDevices()
{
    return devices;
}

std::shared_ptr<const DiskLayout> Datastore::getDiskLayout()
{
    return diskLayout;
}

std::shared_ptr<const DiskFlux> Datastore::getDiskFlux()
{
    return diskFlux;
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

static void rebuildDiskFluxIndices()
{
    sectorByPhysicalLocation.clear();
    sectorByLogicalLocation.clear();
    if (diskFlux)
    {
        for (const auto& track : diskFlux->tracks)
            for (const auto& sector : track->sectors)
            {
                sectorByPhysicalLocation[{sector->physicalCylinder,
                    sector->physicalHead,
                    sector->logicalSector}] = sector;
                sectorByLogicalLocation[{sector->logicalCylinder,
                    sector->logicalHead,
                    sector->logicalSector}] = sector;
            }
    }
}

void Datastore::probeDevices()
{
    Datastore::runOnWorkerThread(
        []
        {
            hex::log::debug("probing USB");
            auto usbDevices = findUsbDevices();

            hex::TaskManager::doLater(
                [usbDevices]
                {
                    devices.clear();
                    devices[DEVICE_FLUXFILE] = {
                        nullptr, "fluxengine.view.config.fluxfile"_lang};
                    devices[DEVICE_MANUAL] = {
                        nullptr, "fluxengine.view.config.manual"_lang};
                    for (auto it : usbDevices)
                        devices["#" + it->serial] = {it,
                            fmt::format(
                                "{} {}", getDeviceName(it->type), it->serial)};
                });
        });
}

void Datastore::rebuildConfiguration()
{
    hex::TaskManager::doLaterOnce(
        []
        {
            configurationValid = false;

            /* Reset and apply the format configuration. */

            auto formatName =
                readSetting<std::string>("format.selected", "ibm");
            if (formatName.empty())
                return;
            Datastore::runOnWorkerThread(
                [=]
                {
                    globalConfig().clear();
                    globalConfig().readBaseConfigFile("_global_options");
                    globalConfig().readBaseConfigFile(formatName);
                });

            /* Device-specific settings. */

            auto device = readSetting<std::string>("device", "fluxfile");
            if (device.empty())
                return;
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
            else
            {
                auto highDensity = readSetting<bool>("highDensity", true);
                Datastore::runOnWorkerThread(
                    [=]
                    {
                        globalConfig()
                            .overrides()
                            ->mutable_drive()
                            ->set_high_density(highDensity);
                    });

                if (device[0] == '#')
                {
                    auto serial = device.substr(1);
                    Datastore::runOnWorkerThread(
                        [=]
                        {
                            globalConfig()
                                .overrides()
                                ->mutable_usb()
                                ->set_serial(serial);
                            globalConfig().setFluxSink("drive:0");
                            globalConfig().setFluxSource("drive:0");
                            globalConfig().setVerificationFluxSource("drive:0");
                        });
                }
            }

            /* Selected options. */

            auto options =
                stringToOptions(readSetting<std::string>("globalSettings", ""));
            options.merge(
                stringToOptions(readSetting<std::string>(formatName, "")));

            Datastore::runOnWorkerThread(
                [=]
                {
                    for (const auto& [key, value] : options)
                        globalConfig().applyOption(key, value);
                });

            /* Update the UI-thread copy of the bits of configuration we
             * need. */

            formattingSupported = false;
            Datastore::runOnWorkerThread(
                []
                {
                    bool formattingSupported;
                    try
                    {
                        auto filesystem = Filesystem::createFilesystem(
                            globalConfig()->filesystem(), nullptr);
                        uint32_t flags = filesystem->capabilities();
                        formattingSupported = flags & Filesystem::OP_CREATE;
                    }
                    catch (const ErrorException&)
                    {
                        formattingSupported = false;
                    }

                    hex::TaskManager::doLater(
                        [=]
                        {
                            ::formattingSupported = formattingSupported;
                        });
                });

            Datastore::runOnWorkerThread(
                []
                {
                    auto diskLayout = createDiskLayout();
                    auto locations = Layout::computePhysicalLocations();
                    auto diskPhysicalBounds = Layout::getBounds(locations);
                    auto diskLogicalBounds =
                        Layout::getBounds(Layout::computeLogicalLocations());

                    decltype(::physicalCylinderLayouts) physicalCylinderLayouts;
                    for (auto& it : locations)
                        physicalCylinderLayouts[it] =
                            Layout::getLayoutOfTrackPhysical(it);

                    hex::TaskManager::doLater(
                        [=]
                        {
                            ::diskLayout = diskLayout;
                            ::diskPhysicalBounds = diskPhysicalBounds;
                            ::diskLogicalBounds = diskLogicalBounds;
                            ::physicalCylinderLayouts = physicalCylinderLayouts;
                            rebuildDiskFluxIndices();
                            configurationValid = true;
                        });
                });
        });
}

void Datastore::onLogMessage(const AnyLogMessage& message)
{
    std::visit(
        overloaded{
            /* Fallback --- do nothing */
            [&](const auto& m)
            {
            },

            /* We terminated due to the stop button. */
            [&](std::shared_ptr<const EmergencyStopMessage> m)
            {
            },

            /* A fatal error. */
            [&](std::shared_ptr<const ErrorLogMessage> m)
            {
                hex::ui::ToastError::open(m->message);
            },

            /* Indicates that we're starting a write operation. */
            [&](std::shared_ptr<const BeginWriteOperationLogMessage> m)
            {
                // _statusBar->SetRightLabel(
                //     fmt::format("W {}.{}", m->track, m->head));
                // _imagerPanel->SetVisualiserMode(
                //     m->track, m->head, VISMODE_WRITING);
            },

            [&](std::shared_ptr<const EndWriteOperationLogMessage> m)
            {
                // _statusBar->SetRightLabel("");
                // _imagerPanel->SetVisualiserMode(0, 0, VISMODE_NOTHING);
            },

            /* Indicates that we're starting a read operation. */
            [&](std::shared_ptr<const BeginReadOperationLogMessage> m)
            {
                // _statusBar->SetRightLabel(
                //     fmt::format("R {}.{}", m->track, m->head));
                // _imagerPanel->SetVisualiserMode(
                //     m->track, m->head, VISMODE_READING);
            },

            [&](std::shared_ptr<const EndReadOperationLogMessage> m)
            {
                // _statusBar->SetRightLabel("");
                // _imagerPanel->SetVisualiserMode(0, 0, VISMODE_NOTHING);
            },

            [&](std::shared_ptr<const TrackReadLogMessage> m)
            {
                // _imagerPanel->SetVisualiserTrackData(m->track);
            },

            [&](std::shared_ptr<const DiskReadLogMessage> m)
            {
                diskFlux = m->disk;
                rebuildDiskFluxIndices();
            },

            /* Large-scale operation start. */
            [&](std::shared_ptr<const BeginOperationLogMessage> m)
            {
                // _statusBar->SetLeftLabel(m->message);
                // _statusBar->ShowProgressBar();
            },

            /* Large-scale operation end. */
            [&](std::shared_ptr<const EndOperationLogMessage> m)
            {
                // _statusBar->SetLeftLabel(m->message);
                // _statusBar->HideProgressBar();
            },

            /* Large-scale operation progress. */
            [&](std::shared_ptr<const OperationProgressLogMessage> m)
            {
                // _statusBar->SetProgress(m->progress);
            },

        },
        message);
}

void Datastore::beginRead(void)
{
    Datastore::runOnWorkerThread(
        []
        {
            busy = true;

            auto fluxSource = FluxSource::create(globalConfig());
            auto decoder = Arch::createDecoder(globalConfig());

            auto diskflux = readDiskCommand(*fluxSource, *decoder);
        });
}

void Datastore::stop(void)
{
    emergencyStop = true;
}

void Datastore::writeImage(const std::fs::path& path)
{
    busy = true;
    Datastore::runOnWorkerThread(
        [=]
        {
            globalConfig().setImageWriter(path.string());
            ImageWriter::create(globalConfig())
                ->writeImage(*Datastore::getDiskFlux()->image);
        });
}

void Datastore::writeFluxFile(const std::fs::path& path)
{
    busy = true;
    Datastore::runOnWorkerThread(
        [=]
        {
            ON_SCOPE_EXIT
            {
                rebuildConfiguration();
            };
            globalConfig().setFluxSink(path.string());
            auto fluxSource = FluxSource::createMemoryFluxSource(*diskFlux);
            auto fluxSink = FluxSink::create(globalConfig());
            writeRawDiskCommand(*fluxSource, *fluxSink);
        });
}

void Datastore::createBlankImage()
{
    busy = true;
    Datastore::runOnWorkerThread(
        [=]
        {
            ON_SCOPE_EXIT
            {
                rebuildConfiguration();
            };
        });
}
