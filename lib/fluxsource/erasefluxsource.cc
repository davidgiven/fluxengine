#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"

class EraseFluxSource : public TrivialFluxSource
{
public:
    EraseFluxSource(const EraseFluxSourceProto& config)
    {
        _extraConfig.mutable_drive()->set_tracks("c0-255h0-1");
    }

    ~EraseFluxSource() {}

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        return std::unique_ptr<const Fluxmap>();
    }

    void recalibrate() override {}
};

std::unique_ptr<FluxSource> FluxSource::createEraseFluxSource(
    const EraseFluxSourceProto& config)
{
    return std::make_unique<EraseFluxSource>(config);
}
