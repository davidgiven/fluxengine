#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "logger.h"
#include "proto.h"
#include "usb/usb.h"
#include "fluxsink/fluxsink.h"
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
			nanoseconds_t oneRevolution;
			int retries = 5;
			usbSetDrive(config.drive().drive(), config.drive().high_density(), config.drive().index_mode());
			Logger() << BeginSpeedOperationLogMessage();

			do {
				oneRevolution = usbGetRotationalPeriod(config.drive().hard_sector_count());
				_hardSectorThreshold = oneRevolution * 3 / (4 * config.drive().hard_sector_count());
				retries--;
			} while ((oneRevolution == 0) && (retries > 0));

			if (oneRevolution == 0) {
				Error() << "Failed\nIs a disk in the drive?";
			}

			Logger() << EndSpeedOperationLogMessage { oneRevolution };
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



