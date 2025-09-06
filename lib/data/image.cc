#include "lib/core/globals.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/config/config.h"

Image::Image() {}

Image::Image(std::set<std::shared_ptr<const Sector>>& sectors)
{
    for (auto& sector : sectors)
    {
        key_t key = std::make_tuple(
            sector->logicalTrack, sector->logicalSide, sector->logicalSector);
        _sectors[key] = sector;
    }
    calculateSize();

    /* The filesystem order is constant for a given layout and so can be
     * computed once. */

    auto& layout = globalConfig()->layout();
    if (layout.has_tracks() && layout.has_sides())
    {
        unsigned block = 0;
        for (const auto& p :
            Layout::getTrackOrdering(layout.filesystem_track_order(),
                layout.tracks(),
                layout.sides()))
        {
            int track = p.first;
            int side = p.second;

            auto trackLayout = Layout::getLayoutOfTrack(track, side);
            if (trackLayout->numSectors == 0)
                continue;

            for (int sectorId : trackLayout->filesystemSectorOrder)
                _filesystemOrder.push_back(
                    std::make_tuple(track, side, sectorId));
        }
    }
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
    key_t key = std::make_tuple(track, side, sectorid);
    return _sectors.find(key) != _sectors.end();
}

std::shared_ptr<const Sector> Image::get(
    unsigned track, unsigned side, unsigned sectorid) const
{
    key_t key = std::make_tuple(track, side, sectorid);
    auto i = _sectors.find(key);
    if (i == _sectors.end())
        return nullptr;
    return i->second;
}

std::shared_ptr<Sector> Image::put(
    unsigned track, unsigned side, unsigned sectorid)
{
    auto trackLayout = Layout::getLayoutOfTrack(track, side);
    key_t key = std::make_tuple(track, side, sectorid);
    std::shared_ptr<Sector> sector = std::make_shared<Sector>();
    sector->logicalTrack = track;
    sector->logicalSide = side;
    sector->logicalSector = sectorid;
    sector->physicalTrack = Layout::remapTrackLogicalToPhysical(track);
    sector->physicalSide = side;
    _sectors[key] = sector;
    return sector;
}

Image::key_t Image::findBlock(unsigned block) const
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
    key_t key = std::make_tuple(track, side, sectorid);
    _sectors.erase(key);
}

std::set<std::pair<unsigned, unsigned>> Image::tracks() const
{
    std::set<std::pair<unsigned, unsigned>> result;
    for (const auto& e : _sectors)
        result.insert(
            std::make_pair(std::get<0>(e.first), std::get<1>(e.first)));
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
