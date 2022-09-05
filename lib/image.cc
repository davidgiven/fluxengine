#include "globals.h"
#include "sector.h"
#include "image.h"

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
}

void Image::clear()
{
    _sectors.clear();
    _geometry = {0, 0, 0};
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
    static std::shared_ptr<const Sector> NONE;

    key_t key = std::make_tuple(track, side, sectorid);
    auto i = _sectors.find(key);
    if (i == _sectors.end())
        return NONE;
    return i->second;
}

std::shared_ptr<Sector> Image::put(
    unsigned track, unsigned side, unsigned sectorid)
{
    key_t key = std::make_tuple(track, side, sectorid);
    std::shared_ptr<Sector> sector = std::make_shared<Sector>();
    sector->logicalTrack = track;
    sector->logicalSide = side;
    sector->logicalSector = sectorid;
    _sectors[key] = sector;
    return sector;
}

void Image::erase(unsigned track, unsigned side, unsigned sectorid)
{
    key_t key = std::make_tuple(track, side, sectorid);
    _sectors.erase(key);
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
