#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxreader.h"

static IntFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1);

static bool high_density = false;

void setHardwareFluxReaderDensity(bool high_density)
{
	::high_density = high_density;
}

class HardwareFluxReader : public FluxReader
{
public:
    HardwareFluxReader(unsigned drive):
        _drive(drive)
    {
    }

    ~HardwareFluxReader()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        usbSetDrive(_drive, high_density);
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

void setHardwareFluxReaderRevolutions(int revolutions)
{
    ::revolutions.value = ::revolutions.defaultValue = revolutions;
}

std::unique_ptr<FluxReader> FluxReader::createHardwareFluxReader(unsigned drive)
{
    return std::unique_ptr<FluxReader>(new HardwareFluxReader(drive));
}



