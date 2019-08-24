#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "fluxsource/fluxsource.h"

class StreamFluxSource : public FluxSource
{
public:
    StreamFluxSource(const std::string& path):
        _path(path)
    {
    }

    ~StreamFluxSource()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        return readStream(_path, track, side);
    }

    void recalibrate() {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createStreamFluxSource(const std::string& path)
{
    return std::unique_ptr<FluxSource>(new StreamFluxSource(path));
}
