#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/logger.h"
#include "lib/config/proto.h"
#include "lib/usb/usb.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/fluxsink/fluxsink.pb.h"

class HardwareSink : public FluxSink
{
    void addFlux(int track, int side, const Fluxmap& fluxmap) override
    {
        auto& drive = globalConfig()->drive();
        usbSetDrive(drive.drive(), drive.high_density(), drive.index_mode());
        usbSeek(track);

        return usbWrite(
            side, fluxmap.rawBytes(), drive.hard_sector_threshold_ns());
    }
};

class HardwareFluxSinkFactory : public FluxSinkFactory
{
public:
    std::unique_ptr<FluxSink> create() override
    {
        return std::make_unique<HardwareSink>();
    }

    bool isHardware() const override
    {
        return true;
    }

    operator std::string() const override
    {
        return "hardware {}";
    }
};

std::unique_ptr<FluxSinkFactory> FluxSinkFactory::createHardwareFluxSinkFactory(
    const HardwareFluxSinkProto& config)
{
    return std::unique_ptr<FluxSinkFactory>();
}
