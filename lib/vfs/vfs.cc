#include "globals.h"
#include "vfs.h"
#include "lib/proto.h"
#include "lib/layout.pb.h"
#include "lib/imginputoutpututils.h"
#include "lib/image.h"
#include "lib/sector.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/config.pb.h"
#include "lib/utils.h"

Path::Path(const std::string& path)
{
    if (path == "")
        return;
    auto p = path;
    if (p[0] == '/')
        p = p.substr(1);

    std::stringstream ss(p);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (item.empty())
            throw BadPathException();
        push_back(item);
    }
}

std::string Path::to_str() const
{
    return join(*this, "/");
}

Filesystem::Filesystem(std::shared_ptr<SectorInterface> sectors):
    _sectors(sectors)
{
    auto& layout = config.layout();

    if (!layout.has_tracks() || !layout.has_sides())
        Error() << "filesystem support cannot be used without concrete layout "
                   "information";

    unsigned block = 0;
    for (const auto& p :
        getTrackOrdering(layout, layout.tracks(), layout.sides()))
    {
        int track = p.first;
        int side = p.second;

        auto trackdata = getTrackFormat(layout, track, side);
        auto sectors = getTrackSectors(trackdata);
        if (sectors.empty())
            Error() << "filesystem support cannot be used without concrete "
                       "layout information";

        for (int sectorId : sectors)
            _locations.push_back(std::make_tuple(track, side, sectorId));
    }
}

std::unique_ptr<Filesystem> Filesystem::createFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> image)
{
    switch (config.filesystem_case())
    {
        case FilesystemProto::kBrother120:
            return Filesystem::createBrother120Filesystem(config, image);

        case FilesystemProto::kAcorndfs:
            return Filesystem::createAcornDfsFilesystem(config, image);

        case FilesystemProto::kFatfs:
            return Filesystem::createFatFsFilesystem(config, image);

        default:
            Error() << "no filesystem configured";
            return std::unique_ptr<Filesystem>();
    }
}

Bytes Filesystem::getLogicalSector(uint32_t number, uint32_t count)
{
    if ((number + count) > _locations.size())
        throw BadFilesystemException();

    Bytes data;
    ByteWriter bw(data);
    for (int i = 0; i < count; i++)
    {
        auto& it = _locations[number + i];
        bw += _sectors->get(std::get<0>(it), std::get<1>(it), std::get<2>(it))
                  ->data;
    }
    return data;
}

void Filesystem::putLogicalSector(uint32_t number, const Bytes& data)
{
    if (number >= _locations.size())
        throw BadFilesystemException();

    auto& i = _locations[number];
    _sectors->put(std::get<0>(i), std::get<1>(i), std::get<2>(i))->data = data;
}

unsigned Filesystem::getLogicalSectorCount()
{
    return _locations.size();
}

unsigned Filesystem::getLogicalSectorSize()
{
    auto trackdata = getTrackFormat(config.layout(), 0, 0);
    return trackdata.sector_size();
}
