#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxwriter.h"

static bool high_density = false;

void setHardwareFluxWriterDensity(bool high_density)
{
	::high_density = high_density;
}

class HardwareFluxWriter : public FluxWriter
{
public:
    HardwareFluxWriter(unsigned drive):
        _drive(drive)
    {
    }

    ~HardwareFluxWriter()
    {
    }

public:
    void writeFlux(int track, int side, Fluxmap& fluxmap)
    {
        usbSetDrive(_drive, high_density);
        usbSeek(track);
        return usbWrite(side, fluxmap);
    }

private:
    unsigned _drive;
};

std::unique_ptr<FluxWriter> FluxWriter::createHardwareFluxWriter(unsigned drive)
{
    return std::unique_ptr<FluxWriter>(new HardwareFluxWriter(drive));
}



