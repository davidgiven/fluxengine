#include "globals.h"
#include "fluxmap.h"
#include "fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

class EraseFluxSource : public FluxSource
{
public:
    EraseFluxSource(const EraseFluxSourceProto& config) {}
    ~EraseFluxSource() {}

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
		return std::unique_ptr<Fluxmap>();
    }

    void recalibrate() {}
};

std::unique_ptr<FluxSource> FluxSource::createEraseFluxSource(const EraseFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new EraseFluxSource(config));
}



