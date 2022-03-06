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
private:
    class HardwareFluxSourceIterator : public FluxSourceIterator
    {
    public:
        HardwareFluxSourceIterator(
            const HardwareFluxSource& fluxsource, int cylinder, int head):
            _fluxsource(fluxsource),
            _cylinder(cylinder),
            _head(head)
        {
        }

        bool hasNext() const
        {
            return true;
        }

        std::unique_ptr<const Fluxmap> next()
        {
            usbSetDrive(_fluxsource._config.drive(),
                _fluxsource._config.high_density(),
                _fluxsource._config.index_mode());
            usbSeek(_cylinder);

            Bytes data = usbRead(_head,
                _fluxsource._config.sync_with_index(),
                _fluxsource._config.revolutions() * _fluxsource._oneRevolution,
                _fluxsource._hardSectorThreshold);
            auto fluxmap = std::make_unique<Fluxmap>();
            fluxmap->appendBytes(data);
            return fluxmap;
        }

    private:
        const HardwareFluxSource& _fluxsource;
        int _cylinder;
        int _head;
    };

public:
    HardwareFluxSource(const HardwareFluxSourceProto& config): _config(config)
    {
        int retries = 5;
        usbSetDrive(
            _config.drive(), _config.high_density(), _config.index_mode());
        Logger() << BeginSpeedOperationLogMessage();

        do
        {
            _oneRevolution =
                usbGetRotationalPeriod(_config.hard_sector_count());
            if (_config.hard_sector_count() != 0)
                _hardSectorThreshold =
                    _oneRevolution * 3 / (4 * _config.hard_sector_count());
            else
                _hardSectorThreshold = 0;

            retries--;
        } while ((_oneRevolution == 0) && (retries > 0));

        if (_oneRevolution == 0)
            Error() << "Failed\nIs a disk in the drive?";

        Logger() << EndSpeedOperationLogMessage{_oneRevolution};
    }

    ~HardwareFluxSource() {}

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int cylinder, int head) override
    {
        return std::make_unique<HardwareFluxSourceIterator>(
            *this, cylinder, head);
    }

    void recalibrate() override
    {
        usbRecalibrate();
    }

    bool isHardware() override
    {
        return true;
    }

private:
    const HardwareFluxSourceProto& _config;
    nanoseconds_t _oneRevolution;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(
    const HardwareFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(config));
}
