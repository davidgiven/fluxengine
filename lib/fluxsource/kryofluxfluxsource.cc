#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/locations.h"
#include "lib/external/kryoflux.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsource/fluxsource.h"
#include <filesystem>

class KryofluxFluxSource : public TrivialFluxSource
{
public:
    KryofluxFluxSource(const KryofluxFluxSourceProto& config):
        _path(config.directory())
    {
        std::vector<CylinderHead> chs;
        for (const auto& di : std::filesystem::directory_iterator(_path))
        {
            static const std::regex FILENAME_REGEX("([0-9]+)\\.([0-9]+)\\.raw");

            std::string filename = di.path().filename();
            std::smatch dmatch;
            if (std::regex_match(filename, dmatch, FILENAME_REGEX))
                chs.push_back(CylinderHead{(unsigned)std::stoi(dmatch[1]),
                    (unsigned)std::stoi(dmatch[2])});
        }
        _extraConfig.mutable_drive()->set_tracks(
            convertCylinderHeadsToString(chs));
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        return readStream(_path, track, side);
    }

    void recalibrate() override {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createKryofluxFluxSource(
    const KryofluxFluxSourceProto& config)
{
    return std::make_unique<KryofluxFluxSource>(config);
}
