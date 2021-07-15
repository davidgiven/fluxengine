#include "globals.h"
#include "sector.h"
#include "image.h"

Image::Image()
{}

Image::Image(std::set<std::shared_ptr<Sector>>& sectors)
{
	for (std::shared_ptr<Sector> sector : sectors)
	{
		key_t key = std::make_tuple(sector->logicalTrack, sector->logicalSide, sector->logicalSector);
		_sectors[key] = sector;
	}
	calculateSize();
}

Sector* Image::get(unsigned track, unsigned side, unsigned sectorid) const
{
	key_t key = std::make_tuple(track, side, sectorid);
	auto i = _sectors.find(key);
	if (i == _sectors.end())
		return nullptr;
	return i->second.get();
}

Sector* Image::put(unsigned track, unsigned side, unsigned sectorid)
{
	key_t key = std::make_tuple(track, side, sectorid);
	std::shared_ptr<Sector> sector = std::make_shared<Sector>();
	_sectors[key] = sector;
	return sector.get();
}

void Image::calculateSize()
{
	_geometry = {};
	for (const auto& i : _sectors)
	{
		const auto& sector = i.second;
		if (sector)
		{
			_geometry.numTracks = std::max(_geometry.numTracks, (unsigned)sector->logicalTrack+1);
			_geometry.numSides = std::max(_geometry.numSides, (unsigned)sector->logicalSide+1);
			_geometry.numSectors = std::max(_geometry.numSectors, (unsigned)sector->logicalSector+1);
			_geometry.sectorSize = std::max(_geometry.sectorSize, (unsigned)sector->data.size());
		}
	}
}

