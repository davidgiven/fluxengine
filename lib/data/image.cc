#include "lib/core/globals.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/config/config.h"

Image::Image() {}

Image::Image(const std::vector<std::shared_ptr<const Sector>>& sectors)
{
    for (auto& sector : sectors)
        _sectors[{sector->logicalCylinder,
            sector->logicalHead,
            sector->logicalSector}] = sector;

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

bool Image::contains(const LogicalLocation& location) const
{
    return _sectors.find(location) != _sectors.end();
}

std::shared_ptr<const Sector> Image::get(const LogicalLocation& location) const
{
    auto i = _sectors.find(location);
    if (i == _sectors.end())
        return nullptr;
    return i->second;
}

std::shared_ptr<Sector> Image::put(const LogicalLocation& location)
{
    auto sector = std::make_shared<Sector>(location);
    _sectors[location] = sector;
    return sector;
}

void Image::erase(const LogicalLocation& location)
{
    _sectors.erase(location);
}

void Image::addMissingSectors(const DiskLayout& diskLayout)
{
    for (auto& location : diskLayout.logicalSectorLocationsInFilesystemOrder)
        if (!_sectors.contains(location))
        {
            auto& ltl = diskLayout.layoutByLogicalLocation.at(
                {location.logicalCylinder, location.logicalHead});
            auto sector = std::make_shared<Sector>(location);
            sector->status = Sector::MISSING;
            _sectors[location] = sector;
        }
    calculateSize();
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
            _geometry.numCylinders = std::max(
                _geometry.numCylinders, (unsigned)sector->logicalCylinder + 1);
            _geometry.numHeads =
                std::max(_geometry.numHeads, (unsigned)sector->logicalHead + 1);
            _geometry.firstSector = std::min(
                _geometry.firstSector, (unsigned)sector->logicalSector);
            maxSector = std::max(maxSector, (unsigned)sector->logicalSector);
            _geometry.sectorSize =
                std::max(_geometry.sectorSize, sector->data.size());
            _geometry.totalBytes += _geometry.sectorSize;
        }
    }
    _geometry.numSectors = maxSector - _geometry.firstSector + 1;
}
