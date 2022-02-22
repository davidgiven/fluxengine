#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "logger.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

class HardwareFluxSource : public FluxSource
{
public:
    HardwareFluxSource(const HardwareFluxSourceProto& config):
        _config(config)
    {
        int retries = 5;
        usbSetDrive(_config.drive(), _config.high_density(), _config.index_mode());
		Logger() << BeginSpeedOperationLogMessage();
 
        do {
            _oneRevolution = usbGetRotationalPeriod(_config.hard_sector_count());
            if (_config.hard_sector_count() != 0)
                _hardSectorThreshold = _oneRevolution * 3 / (4 * _config.hard_sector_count());
            else
                _hardSectorThreshold = 0;

            retries--;
        } while ((_oneRevolution == 0) && (retries > 0));

        if (_oneRevolution == 0) {
			Error() << "Failed\nIs a disk in the drive?";
        }

		Logger() << EndSpeedOperationLogMessage { _oneRevolution };
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
    const HardwareFluxSourceProto& _config;
    nanoseconds_t _oneRevolution;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(const HardwareFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(config));
}



