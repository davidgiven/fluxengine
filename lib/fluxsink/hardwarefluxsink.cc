#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxsink.h"

static bool high_density = false;

void setHardwareFluxSinkDensity(bool high_density)
{
	::high_density = high_density;
}

class HardwareFluxSink : public FluxSink
{
public:
    HardwareFluxSink(unsigned drive):
        _drive(drive)
    {
    }

    ~HardwareFluxSink()
    {
    }

public:
    void writeFlux(int track, int side, Fluxmap& fluxmap)
    {
        usbSetDrive(_drive, high_density);
        usbSeek(track);

        Bytes crunched = fluxmap.rawBytes().crunch();
        return usbWrite(side, crunched);
    }

private:
    unsigned _drive;
};

std::unique_ptr<FluxSink> FluxSink::createHardwareFluxSink(unsigned drive)
{
    return std::unique_ptr<FluxSink>(new HardwareFluxSink(drive));
}



