#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb/usb.h"
#include "fluxsink/fluxsink.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "fmt/format.h"

class HardwareFluxSink : public FluxSink
{
public:
    HardwareFluxSink(const HardwareOutputProto& config):
        _config(config)
    {
		if (config.has_hard_sector_count())
		{
			usbSetDrive(_config.drive(), _config.high_density(), _config.index_mode());
			std::cerr << "Measuring rotational speed... " << std::flush;
			nanoseconds_t oneRevolution = usbGetRotationalPeriod(_config.hard_sector_count());
			_hardSectorThreshold = oneRevolution * 3 / (4 * _config.hard_sector_count());
			std::cerr << fmt::format("{}ms\n", oneRevolution / 1e6);
		}
		else
			_hardSectorThreshold = 0;
    }

    ~HardwareFluxSink()
    {
    }

public:
    void writeFlux(int track, int side, Fluxmap& fluxmap)
    {
        usbSetDrive(_config.drive(), _config.high_density(), _config.index_mode());
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
		return fmt::format("drive {}", _config.drive());
	}

private:
    const HardwareOutputProto& _config;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSink> FluxSink::createHardwareFluxSink(const HardwareOutputProto& config)
{
    return std::unique_ptr<FluxSink>(new HardwareFluxSink(config));
}



