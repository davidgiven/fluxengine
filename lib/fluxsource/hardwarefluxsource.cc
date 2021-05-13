#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "flaggroups/fluxsourcesink.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

FlagGroup hardwareFluxSourceFlags = {
    &fluxSourceSinkFlags,
    &usbFlags
};

class HardwareFluxSource : public FluxSource
{
public:
    HardwareFluxSource(const HardwareInput& config):
        _config(config)
    {
        usbSetDrive(_config.drive(), fluxSourceSinkHighDensity, _config.index_mode());
        std::cerr << "Measuring rotational speed... " << std::flush;
        _oneRevolution = usbGetRotationalPeriod(_config.hard_sector_count());
    if (_config.hard_sector_count() != 0)
        _hardSectorThreshold = _oneRevolution * 3 / (4 * _config.hard_sector_count());
    else
        _hardSectorThreshold = 0;
        std::cerr << fmt::format("{}ms\n", _oneRevolution / 1e6);
    }

    ~HardwareFluxSource()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        usbSetDrive(_config.drive(), _config.high_density(), _config.index_mode());
        usbSeek(track);

        Bytes data = usbRead(
            side, _config.sync_with_index(), _config.revolutions() * _oneRevolution, _hardSectorThreshold);
        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBytes(data);
        return fluxmap;
    }

    void recalibrate()
    {
        usbRecalibrate();
    }

    bool retryable()
    {
        return true;
    }

private:
    const HardwareInput& _config;
    nanoseconds_t _oneRevolution;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(const HardwareInput& config)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(config));
}



