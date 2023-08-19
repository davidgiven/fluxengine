#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fluxsource/fluxsource.h"

class KryofluxFluxSource : public TrivialFluxSource
{
public:
    KryofluxFluxSource(const KryofluxFluxSourceProto& config):
        _path(config.directory())
    {
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        return readStream(_path, track, side);
    }

    void recalibrate() {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createKryofluxFluxSource(
    const KryofluxFluxSourceProto& config)
{
    return std::make_unique<KryofluxFluxSource>(config);
}
