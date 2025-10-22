#include <hex/helpers/logger.hpp>
#include <hex/helpers/auto_reset.hpp>
#include <hex/api/imhex_api/hex_editor.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/task_manager.hpp>
#include <popups/popup_question.hpp>
#include <toasts/toast_notification.hpp>
#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/config/config.h"
#include "lib/config/proto.h"
#include "lib/data/disk.h"
#include "lib/data/image.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/usb/usbfinder.h"
#include "lib/vfs/vfs.h"
#include "lib/vfs/sectorinterface.h"
#include "arch/arch.h"
#include "logview.h"
#include "globals.h"
#include "datastore.h"
#include "utils.h"
#include <thread>
#include <mutex>
#include <deque>
#include <semaphore>

/* Worker thread functions are prefixed with wt. */

using hex::operator""_lang;

static std::shared_ptr<const Disk> disk = std::make_shared<Disk>();
static std::shared_ptr<Image> wtImage;

static std::deque<std::function<void()>> pendingTasks;
static std::mutex pendingTasksMutex;
static std::thread workerThread;

static std::binary_semaphore uiThreadSemaphore(false);

static std::atomic<bool> busy;
static std::atomic<bool> failed;

static bool formattingSupported;
static std::map<std::string, Datastore::Device> devices;
static std::shared_ptr<const DiskLayout> diskLayout;

static void wtRebuildConfiguration(bool useCustom);

static void workerThread_cb()
{
    hex::log::debug("worker thread start");

    /* Only call this with pendingTasksMutex taken. */
    auto stopWorkerThread = []
    {
        workerThread.detach();
        workerThread = std::move(std::thread());
        pendingTasks.clear();
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
        catch (const std::exception& e)
        {
            const std::lock_guard<std::mutex> lock(pendingTasksMutex);
            std::string message = e.what();
            hex::TaskManager::doLater(
                [=]
                {
                    hex::ui::ToastError::open(
                        fmt::format("FluxEngine error: {}", message));
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
                        "FluxEngine worker thread died with an unknown "
                        "exception");
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

template <typename T>
T wtRunSynchronouslyOnUiThread(std::function<T()> callback)
{
    T result;
    hex::TaskManager::doLaterOnce(
        [&]()
        {
            result = callback();
            uiThreadSemaphore.release();
        });
    uiThreadSemaphore.acquire();
    return result;
}

template <>
void wtRunSynchronouslyOnUiThread(std::function<void()> callback)
{
    hex::TaskManager::doLaterOnce(
        [&]()
        {
            callback();
            uiThreadSemaphore.release();
        });
    uiThreadSemaphore.acquire();
}

static void wtWaitForUiThreadToCatchUp()
{
    static std::function<void()> cb = []()
    {
    };

    wtRunSynchronouslyOnUiThread(cb);
}

static void wtOperationStop()
{
    OperationState state =
        failed ? OperationState::Failed : OperationState::Succeeded;
    busy = false;
    hex::TaskManager::doLaterOnce(
        [=]()
        {
            Events::OperationStop::post(state);
            Events::DiskActivityNotification::post(
                DiskActivityType::None, 0, 0);
        });
}

template <typename T>
static T readSetting(const std::string& leaf, const T defaultValue)
{
    return hex::ContentRegistry::Settings::read<T>(
        FLUXENGINE_CONFIG, "fluxengine.settings." + leaf, defaultValue);
}

template <typename T>
static T readSettingFromUiThread(
    const std::string& setting, const T defaultValue)
{
    std::function<T()> cb = [&]()
    {
        return readSetting<T>(setting, defaultValue);
    };
    return wtRunSynchronouslyOnUiThread(cb);
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
                            hex::ImHexApi::System::unlockFrameRate();
                            Datastore::onLogMessage(message);
                        });
                });
        });

    probeDevices();

    Events::SeekToSectorViaPhysicalLocation::subscribe(
        [](CylinderHeadSector physicalLocation)
        {
            if (!diskLayout)
                return;
            auto& ptlo = findOptionally(diskLayout->layoutByPhysicalLocation,
                {physicalLocation.cylinder, physicalLocation.head});
            if (ptlo.has_value())
            {
                auto& ptl = *ptlo;
                auto offseto = findOptionally(
                    diskLayout->sectorOffsetByLogicalSectorLocation,
                    {ptl->logicalTrackLayout->logicalCylinder,
                        ptl->logicalTrackLayout->logicalHead,
                        physicalLocation.sector});
                if (offseto.has_value())
                    hex::ImHexApi::HexEditor::setSelection(hex::Region(
                        *offseto, ptl->logicalTrackLayout->sectorSize));
            }
        });

    Events::SeekToTrackViaPhysicalLocation::subscribe(
        [](CylinderHead physicalLocation)
        {
            if (!disk || !diskLayout)
                return;
            auto ptlo = findOptionally(diskLayout->layoutByPhysicalLocation,
                {physicalLocation.cylinder, physicalLocation.head});
            if (!ptlo.has_value())
                return;
            auto ptl = *ptlo;
            auto ltl = ptl->logicalTrackLayout;
            unsigned firstSectorId = ltl->filesystemSectorOrder.front();
            unsigned lastSectorId = ltl->filesystemSectorOrder.back();

            unsigned startOffset =
                diskLayout->sectorOffsetByLogicalSectorLocation.at(
                    {ltl->logicalCylinder, ltl->logicalHead, firstSectorId});
            unsigned endOffset =
                diskLayout->sectorOffsetByLogicalSectorLocation.at(
                    {ltl->logicalCylinder, ltl->logicalHead, lastSectorId}) +
                ltl->sectorSize;

            hex::ImHexApi::HexEditor::setSelection(
                hex::Region{startOffset, endOffset - startOffset});
        });

    runOnWorkerThread(
        []
        {
            wtRebuildConfiguration(true);
        });
}

bool Datastore::isBusy()
{
    return busy;
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

std::shared_ptr<const Disk> Datastore::getDisk()
{
    return disk;
}

static void badConfiguration()
{
    throw ErrorException("internal error: no configuration");
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

static void wtClearDiskData()
{
    hex::TaskManager::doLater(
        []
        {
            ::wtImage = nullptr;
            ::disk = nullptr;
        });
}

void wtRebuildConfiguration(bool withCustom = false)
{
    /* Reset and apply the format configuration. */

    auto formatName =
        readSettingFromUiThread<std::string>("format.selected", "ibm");
    if (formatName.empty())
        return;
    globalConfig().clear();
    globalConfig().readBaseConfigFile("_global_options");
    globalConfig().readBaseConfigFile(formatName);

    /* Device-specific settings. */

    auto device = readSettingFromUiThread<std::string>("device", "fluxfile");
    if (device.empty())
        return;
    if (device == "fluxfile")
    {
        auto fluxfile =
            readSettingFromUiThread<std::fs::path>("fluxfile", "unset")
                .string();
        globalConfig().setFluxSink(fluxfile);
        globalConfig().setFluxSource(fluxfile);
        globalConfig().setVerificationFluxSource(fluxfile);
    }
    else
    {
        auto highDensity = readSettingFromUiThread<bool>("highDensity", true);
        globalConfig().overrides()->mutable_drive()->set_high_density(
            highDensity);

        if (device[0] == '#')
        {
            auto serial = device.substr(1);
            globalConfig().overrides()->mutable_usb()->set_serial(serial);
            globalConfig().setFluxSink("drive:0");
            globalConfig().setFluxSource("drive:0");
            globalConfig().setVerificationFluxSource("drive:0");
        }
    }

    /* Selected options. */

    auto options = stringToOptions(
        readSettingFromUiThread<std::string>("globalSettings", ""));
    options.merge(
        stringToOptions(readSettingFromUiThread<std::string>(formatName, "")));

    for (const auto& [key, value] : options)
        globalConfig().applyOption(key, value);

    /* Custom settings. */

    globalConfig().applyOptionsFile(
        readSettingFromUiThread<std::string>("systemProperties", ""));
    globalConfig().applyOptionsFile(
        readSettingFromUiThread<std::string>("custom", ""));

    /* Finalise the options. */

    globalConfig().applyDefaultOptions();
    globalConfig().validateAndThrow();
    auto diskLayout = createDiskLayout();

    /* Update the UI-thread copy of the bits of configuration we
     * need. */

    bool formattingSupported = false;
    try
    {
        auto filesystem = Filesystem::createFilesystem(
            globalConfig()->filesystem(), diskLayout, nullptr);
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
            ::diskLayout = diskLayout;
        });
}

void Datastore::onLogMessage(const AnyLogMessage& message)
{
    LogView::logMessage(message);
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
                Events::DiskActivityNotification::post(
                    DiskActivityType::Write, m->track, m->head);
            },

            [&](std::shared_ptr<const EndWriteOperationLogMessage> m)
            {
                Events::DiskActivityNotification::post(
                    DiskActivityType::None, 0, 0);
            },

            /* Indicates that we're starting a read operation. */
            [&](std::shared_ptr<const BeginReadOperationLogMessage> m)
            {
                Events::DiskActivityNotification::post(
                    DiskActivityType::Read, m->track, m->head);
            },

            [&](std::shared_ptr<const EndReadOperationLogMessage> m)
            {
                Events::DiskActivityNotification::post(
                    DiskActivityType::None, 0, 0);
            },

            [&](std::shared_ptr<const TrackReadLogMessage> m)
            {
                // _imagerPanel->SetVisualiserTrackData(m->track);
            },

            [&](std::shared_ptr<const DiskReadLogMessage> m)
            {
                /* This is where data gets from the worker thread to the GUI.
                 * The disk here is a copy of the one being worked on, and
                 * is guaranteed not to change. */

                disk = m->disk;
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

void Datastore::beginRead(bool rereadBadSectors)
{
    Events::OperationStart::post("fluxengine.view.status.readDevice"_lang);
    Datastore::runOnWorkerThread(
        [=]
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(true);
                if (!rereadBadSectors)
                    wtClearDiskData();

                std::shared_ptr<Disk> disk;
                wtRunSynchronouslyOnUiThread((std::function<void()>)[&] {
                    if (::disk)
                        disk = std::make_shared<Disk>(*::disk);
                    else
                        disk = std::make_shared<Disk>();
                });
                auto fluxSource = FluxSource::create(globalConfig());
                auto decoder = Arch::createDecoder(globalConfig());

                readDiskCommand(*diskLayout, *fluxSource, *decoder, *disk);
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::beginWrite()
{
    Events::OperationStart::post("fluxengine.view.status.writeDevice"_lang);
    Datastore::runOnWorkerThread(
        []
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(true);
                wtWaitForUiThreadToCatchUp();

                auto fluxSinkFactory = FluxSinkFactory::create(globalConfig());
                auto encoder = Arch::createEncoder(globalConfig());
                std::shared_ptr<Decoder> decoder;
                std::shared_ptr<FluxSource> verificationFluxSource;
                if (globalConfig().hasDecoder() &&
                    fluxSinkFactory->isHardware())
                {
                    decoder = Arch::createDecoder(globalConfig());
                    verificationFluxSource = FluxSource::create(
                        globalConfig().getVerificationFluxSourceProto());
                }

                auto path = fluxSinkFactory->getPath();
                if (path.has_value() && std::filesystem::exists(*path))
                {
                    {
                        bool result;
                        wtRunSynchronouslyOnUiThread((
                            std::function<void()>)[&] {
                            hex::ui::PopupQuestion::open(
                                "fluxengine.messages.writingFluxToFile"_lang,
                                [&]
                                {
                                    result = true;
                                },
                                [&]
                                {
                                    result = false;
                                });
                        });
                        if (!result)
                            throw EmergencyStopException();
                    }
                }

                auto image = disk->image;
                writeDiskCommand(*diskLayout,
                    *image,
                    *encoder,
                    *fluxSinkFactory,
                    decoder.get(),
                    verificationFluxSource.get());
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::reset()
{
    Events::OperationStart::post("");
    Datastore::runOnWorkerThread(
        []
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(false);
                wtClearDiskData();
                wtWaitForUiThreadToCatchUp();
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::stop()
{
    emergencyStop = true;
}

static std::shared_ptr<Disk> wtMakeDiskDataFromImage(
    std::shared_ptr<Image>& image)
{
    image->calculateSize();
    image->populateSectorPhysicalLocationsFromLogicalLocations(*diskLayout);
    if (image->getGeometry().totalBytes != diskLayout->totalBytes)
        error("loaded image is not the right size for this format");

    return std::make_shared<Disk>(image, *diskLayout);
}

void Datastore::writeImage(const std::fs::path& path)
{
    Events::OperationStart::post("fluxengine.view.status.writeImage"_lang);
    Datastore::runOnWorkerThread(
        [=]
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(true);
                wtWaitForUiThreadToCatchUp();
                globalConfig().setImageWriter(path.string());
                ImageWriter::create(globalConfig())
                    ->writeImage(*Datastore::getDisk()->image);
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::readImage(const std::fs::path& path)
{
    Events::OperationStart::post("fluxengine.view.status.readImage"_lang);
    Datastore::runOnWorkerThread(
        [=]
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(true);
                wtWaitForUiThreadToCatchUp();
                globalConfig().setImageReader(path.string());
                auto imageReader = ImageReader::create(globalConfig());
                std::shared_ptr<Image> image = imageReader->readImage();

                const auto& extraConfig = imageReader->getExtraConfig();
                auto customConfig = renderProtoAsConfig(&extraConfig);

                /* Update the setting, and then rebuild the config again as it
                 * will have changed. */

                wtRunSynchronouslyOnUiThread((std::function<void()>)[=] {
                    Events::SetSystemConfig::post(customConfig);
                });
                wtRebuildConfiguration(true);
                wtWaitForUiThreadToCatchUp();

                auto disk = wtMakeDiskDataFromImage(image);
                hex::TaskManager::doLater(
                    [=]
                    {
                        ::disk = disk;
                    });
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::writeFluxFile(const std::fs::path& path)
{
    Events::OperationStart::post("fluxengine.view.status.writeFlux"_lang);
    Datastore::runOnWorkerThread(
        [=]
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(true);
                wtWaitForUiThreadToCatchUp();

                if (!disk || !disk->image)
                    error("no loaded image");
                if (disk->image->getGeometry().totalBytes !=
                    diskLayout->totalBytes)
                    error(
                        "loaded image is not the right size for this "
                        "format");

                globalConfig().setFluxSink(path.string());
                auto fluxSource = FluxSource::createMemoryFluxSource(*disk);
                auto fluxSinkFactory = FluxSinkFactory::create(globalConfig());
                writeRawDiskCommand(*diskLayout, *fluxSource, *fluxSinkFactory);
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}

void Datastore::createBlankImage()
{
    Events::OperationStart::post("fluxengine.view.status.blankFilesystem"_lang);
    Datastore::runOnWorkerThread(
        [=]
        {
            busy = true;
            failed = false;
            DEFER(wtOperationStop());

            try
            {
                wtRebuildConfiguration(false);
                wtClearDiskData();
                wtWaitForUiThreadToCatchUp();

                auto image = std::make_shared<Image>();
                std::shared_ptr<SectorInterface> sectorInterface =
                    SectorInterface::createMemorySectorInterface(image);
                auto filesystem = Filesystem::createFilesystem(
                    globalConfig()->filesystem(), diskLayout, sectorInterface);

                filesystem->create(false, "FLUXENGINE");
                filesystem->flushChanges();

                auto disk = wtMakeDiskDataFromImage(image);
                hex::TaskManager::doLater(
                    [=]
                    {
                        Events::SetSystemConfig::post("");
                        ::disk = disk;
                    });
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        });
}
