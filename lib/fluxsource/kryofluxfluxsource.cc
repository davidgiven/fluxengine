#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fluxsource/fluxsource.h"

class KryofluxFluxSource : public FluxSource
{
public:
    KryofluxFluxSource(const KryofluxFluxSourceProto& config):
        _path(config.directory())
    {}

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        return readStream(_path, track, side);
    }

    void recalibrate() {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createKryofluxFluxSource(const KryofluxFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new KryofluxFluxSource(config));
}
