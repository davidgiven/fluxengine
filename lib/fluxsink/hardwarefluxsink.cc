#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "logger.h"
#include "proto.h"
#include "usb/usb.h"
#include "fluxsink/fluxsink.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/readerwriter.h"

class HardwareFluxSink : public FluxSink
{
public:
    HardwareFluxSink(const HardwareFluxSinkProto& conf): _config(conf) {}

    ~HardwareFluxSink() {}

public:
    void writeFlux(int track, int side, const Fluxmap& fluxmap) override
    {
        auto& drive = globalConfig()->drive();
        usbSetDrive(drive.drive(), drive.high_density(), drive.index_mode());
        usbSeek(track);

        return usbWrite(
            side, fluxmap.rawBytes(), drive.hard_sector_threshold_ns());
    }

    bool isHardware() const override
    {
        return true;
    }

    operator std::string() const override
    {
        return fmt::format("drive {}", globalConfig()->drive().drive());
    }

private:
    const HardwareFluxSinkProto& _config;
};

std::unique_ptr<FluxSink> FluxSink::createHardwareFluxSink(
    const HardwareFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new HardwareFluxSink(config));
}
