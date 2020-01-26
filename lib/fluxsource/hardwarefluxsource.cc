#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxsource/fluxsource.h"

FlagGroup hardwareFluxSourceFlags;

static IntFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1);

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
    }

    ~HardwareFluxSource()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        usbSetDrive(_drive, high_density, indexMode);
        usbSeek(track);
        Bytes crunched = usbRead(side, revolutions);
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
};

void setHardwareFluxSourceRevolutions(int revolutions)
{
    ::revolutions.setDefaultValue(revolutions);
}

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(unsigned drive)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(drive));
}



