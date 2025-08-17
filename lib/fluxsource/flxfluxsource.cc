#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/locations.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/external/flx.h"
#include "lib/core/logger.h"
#include <filesystem>

class FlxFluxSource : public TrivialFluxSource
{
public:
    FlxFluxSource(const FlxFluxSourceProto& config): _path(config.directory())
    {
        std::vector<CylinderHead> chs;
        for (const auto& di : std::filesystem::directory_iterator(_path))
        {
            static const std::regex FILENAME_REGEX(
                "@TR([0-9]+)S([0-9]+)@\\.FLX");

            std::string filename = di.path().filename().string();
            std::smatch dmatch;
            if (std::regex_match(filename, dmatch, FILENAME_REGEX))
                chs.push_back(CylinderHead{(unsigned)std::stoi(dmatch[1]),
                    (unsigned)std::stoi(dmatch[2]) - 1});
        }
        _extraConfig.mutable_drive()->set_tracks(
            convertCylinderHeadsToString(chs));
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        std::string path =
            fmt::format("{}/@TR{:02}S{}@.FLX", _path, track, side + 1);
        log("FLX: reading {}", path);
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
