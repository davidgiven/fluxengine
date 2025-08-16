#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/locations.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/external/catweasel.h"
#include "lib/config/proto.h"
#include "lib/core/logger.h"
#include <fstream>
#include <filesystem>

class DmkFluxSourceIterator : public FluxSourceIterator
{
public:
    DmkFluxSourceIterator(const std::string& path, int track, int side):
        _path(path),
        _track(track),
        _side(side)
    {
    }

    bool hasNext() const override
    {
        std::string p = getPath();
        return std::filesystem::exists(getPath());
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        std::string path = getPath();
        log("DMK: reading {}", path);
        std::ifstream ifstream(getPath(), std::ios::binary);
        if (!ifstream)
            return std::make_unique<Fluxmap>();
        _count++;

        Bytes bytes(ifstream);
        return decodeCatweaselData(bytes, 1e9 / 7080500.0);
    }

private:
    const std::string getPath() const
    {
        return fmt::format(
            "{}/C_S{:01}T{:02}.{:03}", _path, _side, _track, _count);
    }

private:
    const std::string _path;
    const int _track;
    const int _side;
    int _count = 0;
};

class DmkFluxSource : public FluxSource
{
public:
    DmkFluxSource(const DmkFluxSourceProto& config): _path(config.directory())
    {
        std::vector<CylinderHead> chs;
        for (const auto& di : std::filesystem::directory_iterator(_path))
        {
            static const std::regex FILENAME_REGEX(
                "C_S([0-9]+)T([0-9]+)\\.[0-9]+");

            std::string filename = di.path().filename();
            std::smatch dmatch;
            if (std::regex_match(filename, dmatch, FILENAME_REGEX))
                chs.push_back(CylinderHead{(unsigned)std::stoi(dmatch[2]),
                    (unsigned)std::stoi(dmatch[1])});
        }
        _extraConfig.mutable_drive()->set_tracks(
            convertCylinderHeadsToString(chs));
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int side) override
    {
        return std::make_unique<DmkFluxSourceIterator>(_path, track, side);
    }

    void recalibrate() override {}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createDmkFluxSource(
    const DmkFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new DmkFluxSource(config));
}
