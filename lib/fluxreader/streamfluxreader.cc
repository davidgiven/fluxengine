#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "fluxreader.h"

class StreamFluxReader : public FluxReader
{
public:
    StreamFluxReader(const std::string& path):
        _path(path)
    {
    }

    ~StreamFluxReader()
    {
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        return readStream(_path, track, side);
    }

    void recalibrate() {}

private:
    const std::string& _path;
};

std::unique_ptr<FluxReader> FluxReader::createStreamFluxReader(const std::string& path)
{
    return std::unique_ptr<FluxReader>(new StreamFluxReader(path));
}
