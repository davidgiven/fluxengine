#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "usb.h"
#include "fluxsource/fluxsource.h"

FlagGroup quickdiskFluxSourceFlags;

class QuickdiskFluxSource : public FluxSource
{
public:
    QuickdiskFluxSource(unsigned drive):
        _drive(drive)
    {
    }

    ~QuickdiskFluxSource()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        usbSetDrive(_drive, false);
        Bytes crunched = usbReadQD(side);
        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBytes(crunched.uncrunch());
        return fluxmap;
    }

    void recalibrate()
    {
    }

    bool retryable()
    {
        return true;
    }

private:
    unsigned _drive;
};

std::unique_ptr<FluxSource> FluxSource::createQuickdiskFluxSource(unsigned drive)
{
    return std::unique_ptr<FluxSource>(new QuickdiskFluxSource(drive));
}



