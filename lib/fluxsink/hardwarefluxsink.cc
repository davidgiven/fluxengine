#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/fluxmap.h"
#include "lib/logger.h"
#include "lib/proto.h"
#include "lib/usb/usb.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "fmt/format.h"

class HardwareFluxSink : public FluxSink
{
public:
    HardwareFluxSink(const HardwareFluxSinkProto& conf):
        _config(conf)
    {
		if (config.drive().has_hard_sector_count())
		{
			int retries = 5;
			usbSetDrive(config.drive().drive(), config.drive().high_density(), config.drive().index_mode());

			nanoseconds_t oneRevolution = config.drive().rotational_period_ms() * 1e6;
			if (oneRevolution == 0)
			{
				Logger() << BeginSpeedOperationLogMessage();

				do {
					oneRevolution = usbGetRotationalPeriod(config.drive().hard_sector_count());
					_hardSectorThreshold = oneRevolution * 3 / (4 * config.drive().hard_sector_count());
					retries--;
				} while ((oneRevolution == 0) && (retries > 0));
				config.mutable_drive()->set_rotational_period_ms(oneRevolution / 1e6);

				Logger() << EndSpeedOperationLogMessage { oneRevolution };
			}

			if (oneRevolution == 0) {
				Error() << "Failed\nIs a disk in the drive?";
			}
		}
		else
			_hardSectorThreshold = 0;
    }

    ~HardwareFluxSink()
    {
    }

public:
    void writeFlux(int track, int side, const Fluxmap& fluxmap) override
    {
        usbSetDrive(config.drive().drive(), config.drive().high_density(), config.drive().index_mode());
		#if 0
		if (fluxSourceSinkFortyTrack)
		{
			if (track & 1)
				Error() << "cannot write to odd physical tracks in 40-track mode";
			usbSeek(track / 2);
		}
		else
		#endif
			usbSeek(track);

        return usbWrite(side, fluxmap.rawBytes(), _hardSectorThreshold);
    }

	operator std::string () const
	{
		return fmt::format("drive {}", config.drive().drive());
	}

private:
    const HardwareFluxSinkProto& _config;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSink> FluxSink::createHardwareFluxSink(const HardwareFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new HardwareFluxSink(config));
}



