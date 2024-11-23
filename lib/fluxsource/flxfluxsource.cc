#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/external/flx.h"

class FlxFluxSource : public TrivialFluxSource
{
public:
    FlxFluxSource(const FlxFluxSourceProto& config): _path(config.directory())
    {
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        std::string path =
            fmt::format("{}/@TR{:02}S{}@.FLX", _path, track, side + 1);
        return readFlxBytes(Bytes::readFromFile(path));
    }

    void recalibrate() override {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createFlxFluxSource(
    const FlxFluxSourceProto& config)
{
    return std::make_unique<FlxFluxSource>(config);
}
