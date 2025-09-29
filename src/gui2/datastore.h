#pragma once

#include "lib/core/globals.h"
#include "lib/core/logger.h"
#include "lib/data/layout.h"

class DiskFlux;
class DiskLayout;
class CandidateDevice;

static constexpr std::string DEVICE_MANUAL = "manual";
static constexpr std::string DEVICE_FLUXFILE = "fluxfile";

class Datastore
{
public:
    struct Device
    {
        std::shared_ptr<CandidateDevice> coreDevice;
        std::string label;
    };

public:
    static void init();

    static bool isBusy();
    static bool isConfigurationValid();
    static bool canFormat();
    static void probeDevices();

    static const std::map<std::string, Device>& getDevices();
    static const std::map<CylinderHead, std::shared_ptr<const TrackInfo>>&
    getPhysicalCylinderLayouts();
    static std::shared_ptr<const Sector> findSectorByPhysicalLocation(
        const CylinderHeadSector& location);
    static std::shared_ptr<const Sector> findSectorByLogicalLocation(
        const LogicalLocation& location);
    static std::optional<unsigned> findBlockByLogicalLocation(
        const LogicalLocation& location);
    static std::shared_ptr<const DiskLayout> getDiskLayout();
    static void rebuildConfiguration();
    static void onLogMessage(const AnyLogMessage& message);

    static void beginRead();
    static void writeImage(const std::fs::path& path);
    static void writeFluxFile(const std::fs::path& path);
    static void createBlankImage();
    static void stop();

    static std::shared_ptr<const DiskFlux> getDiskFlux();

    static void runOnWorkerThread(std::function<void()> callback);
};