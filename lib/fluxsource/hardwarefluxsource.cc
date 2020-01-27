#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxsource/fluxsource.h"
#include "fmt/format.h"

FlagGroup hardwareFluxSourceFlags;

static DoubleFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1.25);

static BoolFlag synced(
    { "--sync-with-index" },
    "whether to wait for an index pulse before started to read",
    false);

static IntFlag indexMode(
    { "--index-mode" },
    "index pulse source (0=drive, 1=300 RPM fake source, 2=360 RPM fake source",
    0);

static bool high_density = false;

void setHardwareFluxSourceDensity(bool high_density)
{
	::high_density = high_density;
}

class HardwareFluxSource : public FluxSource
{
public:
    HardwareFluxSource(unsigned drive):
        _drive(drive)
    {
        usbSetDrive(_drive, high_density, indexMode);
        std::cerr << "Measuring rotational speed... " << std::flush;
        _oneRevolution = usbGetRotationalPeriod();
        std::cerr << fmt::format("{}ms\n", _oneRevolution / 1e6);
    }

    ~HardwareFluxSource()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        usbSetDrive(_drive, high_density, indexMode);
        usbSeek(track);
        Bytes crunched = usbRead(side, synced, revolutions * _oneRevolution);
        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBytes(crunched.uncrunch());
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
    unsigned _drive;
    unsigned _revolutions;
    nanoseconds_t _oneRevolution;
};

void setHardwareFluxSourceRevolutions(double revolutions)
{
    ::revolutions.setDefaultValue(revolutions);
}

void setHardwareFluxSourceSynced(bool synced)
{
    ::synced.setDefaultValue(synced);
}

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(unsigned drive)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(drive));
}



