#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxreader.h"
#include <fmt/format.h>

static IntFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1);

static SettableFlag highDensity(
    { "--high-density", "--hd" },
    "sets the drive to high density mode");

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
        usbSetDrive(_drive, highDensity);
        usbSeek(track);
        Bytes crunched = usbRead(side, _revolutions);
        std::cout << fmt::format("({} bytes crunched) ", crunched.size());

        Bytes uncrunched = crunched.uncrunch();
        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBytes(uncrunched);
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



