#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb/usb.h"
#include "fluxsink/fluxsink.h"
#include "fmt/format.h"

FlagGroup hardwareFluxSinkFlags = {
	&usbFlags,
};

static bool high_density = false;

static IntFlag indexMode(
    { "--write-index-mode" },
    "index pulse source (0=drive, 1=300 RPM fake source, 2=360 RPM fake source",
    0);

static IntFlag hardSectorCount(
    { "--write-hard-sector-count" },
    "number of hard sectors on the disk (0=soft sectors)",
    0);

static BoolFlag doubleStep(
    { "--write-double-step" },
    "double-step 96tpi drives for 48tpi media",
    false);

static IntFlag stepIntervalTime(
    { "--write-step-interval-time" },
    "Head step interval time in milliseconds",
    6);

static IntFlag stepSettlingTime(
    { "--write-step-settling-time" },
    "Head step settling time in milliseconds",
    50);

void setHardwareFluxSinkDensity(bool high_density)
{
	::high_density = high_density;
}

void setHardwareFluxSinkHardSectorCount(int sectorCount)
{
	::hardSectorCount.setDefaultValue(sectorCount);
}

class HardwareFluxSink : public FluxSink
{
public:
    HardwareFluxSink(unsigned drive):
        _drive(drive)
    {
		if (hardSectorCount != 0)
		{
			usbSetDrive(_drive, high_density, indexMode,
                stepIntervalTime, stepSettlingTime, doubleStep);
			std::cerr << "Measuring rotational speed... " << std::flush;
			nanoseconds_t oneRevolution = usbGetRotationalPeriod(hardSectorCount);
			_hardSectorThreshold = oneRevolution * 3 / (4 * hardSectorCount);
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
        usbSetDrive(_drive, high_density, indexMode,
            stepIntervalTime, stepSettlingTime, doubleStep);
        usbSeek(track);

        return usbWrite(side, fluxmap.rawBytes(), _hardSectorThreshold);
    }

private:
    unsigned _drive;
    nanoseconds_t _hardSectorThreshold;
};

std::unique_ptr<FluxSink> FluxSink::createHardwareFluxSink(unsigned drive)
{
    return std::unique_ptr<FluxSink>(new HardwareFluxSink(drive));
}



