#include "lib/core/globals.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/config/config.h"

Image::Image() {}

Image::Image(std::set<std::shared_ptr<const Sector>>& sectors):
    _filesystemOrder(Layout::computeFilesystemLogicalOrdering())
{
    for (auto& sector : sectors)
        _sectors[{
            sector->logicalTrack, sector->logicalSide, sector->logicalSector}] =
            sector;
    calculateSize();
}

void Image::clear()
{
    _sectors.clear();
    _geometry = {0, 0, 0};
}

void Image::createBlankImage()
{
    clear();
    for (const auto& trackAndHead : Layout::getTrackOrdering(
             globalConfig()->layout().filesystem_track_order()))
    {
        unsigned track = trackAndHead.first;
        unsigned side = trackAndHead.second;
        auto trackLayout = Layout::getLayoutOfTrack(track, side);
        Bytes blank(trackLayout->sectorSize);
        for (unsigned sectorId : trackLayout->naturalSectorOrder)
            put(track, side, sectorId)->data = blank;
    }
}

bool Image::empty() const
{
    return _sectors.empty();
}

bool Image::contains(unsigned track, unsigned side, unsigned sectorid) const
{
    return _sectors.find({track, side, sectorid}) != _sectors.end();
}

std::shared_ptr<const Sector> Image::get(
    unsigned track, unsigned side, unsigned sectorid) const
{
    auto i = _sectors.find({track, side, sectorid});
    if (i == _sectors.end())
        return nullptr;
    return i->second;
}

std::shared_ptr<Sector> Image::put(
    unsigned track, unsigned side, unsigned sectorid)
{
    auto trackLayout = Layout::getLayoutOfTrack(track, side);
    std::shared_ptr<Sector> sector = std::make_shared<Sector>();
    sector->logicalTrack = track;
    sector->logicalSide = side;
    sector->logicalSector = sectorid;
    sector->physicalTrack = Layout::remapTrackLogicalToPhysical(track);
    sector->physicalSide = side;
    _sectors[{track, side, sectorid}] = sector;
    return sector;
}

CylinderHeadSector Image::findBlock(unsigned block) const
{
    if (block >= _filesystemOrder.size())
        error("block {} is out of bounds ({} maximum)",
            block,
            _filesystemOrder.size());

    return _filesystemOrder.at(block);
}

std::shared_ptr<const Sector> Image::getBlock(unsigned block) const
{
    auto [cylinder, head, sector] = findBlock(block);
    return get(cylinder, head, sector);
}

std::shared_ptr<Sector> Image::putBlock(unsigned block)
{
    auto [cylinder, head, sector] = findBlock(block);
    return put(cylinder, head, sector);
}

int Image::getBlockCount() const
{
    return _filesystemOrder.size();
}

void Image::erase(unsigned track, unsigned side, unsigned sectorid)
{
    _sectors.erase({track, side, sectorid});
}

std::set<CylinderHead> Image::tracks() const
{
    std::set<CylinderHead> result;
    for (const auto& [location, sector] : _sectors)
        result.insert({location.cylinder, location.head});
    return result;
}

void Image::calculateSize()
{
    _geometry = {};
    unsigned maxSector = 0;
    for (const auto& i : _sectors)
    {
        const auto& sector = i.second;
        if (sector)
        {
            _geometry.numTracks = std::max(
                _geometry.numTracks, (unsigned)sector->logicalTrack + 1);
            _geometry.numSides =
                std::max(_geometry.numSides, (unsigned)sector->logicalSide + 1);
            _geometry.firstSector = std::min(
                _geometry.firstSector, (unsigned)sector->logicalSector);
            maxSector = std::max(maxSector, (unsigned)sector->logicalSector);
            _geometry.sectorSize =
                std::max(_geometry.sectorSize, (unsigned)sector->data.size());
        }
    }
    _geometry.numSectors = maxSector - _geometry.firstSector + 1;
}
