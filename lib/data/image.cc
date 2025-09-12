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
        _sectors[{sector->logicalCylinder,
            sector->logicalHead,
            sector->logicalSector}] = sector;
    for (auto& location : _filesystemOrder)
        if (!_sectors.contains(location))
        {
            auto trackInfo = Layout::getLayoutOfTrack(
                location.logicalCylinder, location.logicalHead);
            auto sector = std::make_shared<Sector>(trackInfo, location);
            sector->status = Sector::MISSING;
            _sectors[location] = sector;
        }

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
    std::shared_ptr<Sector> sector = std::make_shared<Sector>(
        trackLayout, LogicalLocation{track, side, sectorid});
    sector->physicalCylinder = Layout::remapCylinderLogicalToPhysical(track);
    sector->physicalHead = side;
    _sectors[{track, side, sectorid}] = sector;
    return sector;
}

LogicalLocation Image::findBlock(unsigned block) const
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

Image::LocationAndOffset Image::findBlockByOffset(unsigned offset) const
{
    for (unsigned block = 0; block < _filesystemOrder.size(); block++)
    {
        auto sector = getBlock(block);
        if (!sector)
            throw OutOfRangeException("sector missing from image");
        if (offset < sector->trackLayout->sectorSize)
            return {block, offset};
        offset -= sector->trackLayout->sectorSize;
    }

    throw OutOfRangeException("location is not in the image");
}

unsigned Image::findOffsetByLogicalLocation(
    const LogicalLocation& logicalLocation) const
{
    unsigned offset = 0;
    for (const auto& it : _filesystemOrder)
    {
        if (it == logicalLocation)
            return offset;
        const auto& ot = _sectors.find(it);
        if (ot == _sectors.end())
            throw OutOfRangeException("sector missing from image");
        const auto& sector = ot->second;
        offset += sector->trackLayout->sectorSize;
    }

    throw OutOfRangeException("location is not in the image");
}

/* TODO: this is all wrong and the whole layout stuff needs to be refactored. */
unsigned Image::findApproximateOffsetByPhysicalLocation(
    const CylinderHeadSector& physicalLocation) const
{
    unsigned offset = 0;
    for (const auto& it : _filesystemOrder)
    {
        const auto& sector = _sectors.at(it);
        CylinderHeadSector sectorPhysicalLocation = {sector->physicalCylinder,
            sector->physicalHead,
            sector->logicalSector};
        if (sectorPhysicalLocation >= physicalLocation)
            break;
        offset += sector->trackLayout->sectorSize;
    }

    return offset;
}

void Image::erase(unsigned track, unsigned side, unsigned sectorid)
{
    _sectors.erase({track, side, sectorid});
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
                std::max(_geometry.sectorSize, sector->trackLayout->sectorSize);
            _geometry.totalBytes += sector->trackLayout->sectorSize;
        }
    }
    _geometry.numSectors = maxSector - _geometry.firstSector + 1;
}
