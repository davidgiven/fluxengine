#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "logger.h"
#include "proto.h"
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
            const HardwareFluxSource& fluxsource, int track, int head):
            _fluxsource(fluxsource),
            _track(track),
            _head(head)
        {
        }

        bool hasNext() const
        {
            return true;
        }

        std::unique_ptr<const Fluxmap> next()
        {
            usbSetDrive(config.drive().drive(), config.drive().high_density(), config.drive().index_mode());
            usbSeek(_track);

            Bytes data = usbRead(_head,
                config.drive().sync_with_index(),
                config.drive().revolutions() * _fluxsource._oneRevolution,
                _fluxsource._hardSectorThreshold);
            auto fluxmap = std::make_unique<Fluxmap>();
            fluxmap->appendBytes(data);
            return fluxmap;
        }

    private:
        const HardwareFluxSource& _fluxsource;
        int _track;
        int _head;
    };

public:
    HardwareFluxSource(const HardwareFluxSourceProto& conf): _config(conf)
    {
        int retries = 5;
        usbSetDrive(config.drive().drive(), config.drive().high_density(), config.drive().index_mode());
        Logger() << BeginSpeedOperationLogMessage();

		_oneRevolution = config.drive().rotational_period_ms() * 1e6;
		if (_oneRevolution == 0)
		{
			do
			{
				_oneRevolution =
					usbGetRotationalPeriod(config.drive().hard_sector_count());
				if (config.drive().hard_sector_count() != 0)
					_hardSectorThreshold =
						_oneRevolution * 3 / (4 * config.drive().hard_sector_count());
				else
					_hardSectorThreshold = 0;

				retries--;
			} while ((_oneRevolution == 0) && (retries > 0));
			config.mutable_drive()->set_rotational_period_ms(_oneRevolution / 1e6);
		}

        if (_oneRevolution == 0)
            Error() << "Failed\nIs a disk in the drive?";

        Logger() << EndSpeedOperationLogMessage{_oneRevolution};
    }

    ~HardwareFluxSource() {}

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int head) override
    {
        return std::make_unique<HardwareFluxSourceIterator>(
            *this, track, head);
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
