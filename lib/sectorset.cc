#include "globals.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"

std::unique_ptr<Sector>& SectorSet::get(int track, int head, int sector)
{
	key_t key(track, head, sector);
	return _data[key];
}

Sector* SectorSet::get(int track, int head, int sector) const
{
    key_t key(track, head, sector);
	auto i = _data.find(key);
	if (i == _data.end())
		return NULL;
	return i->second.get();
}

void SectorSet::calculateSize(
	unsigned& numTracks, unsigned& numHeads,
	unsigned& numSectors, unsigned& sectorSize) const
{
	numTracks = numHeads = numSectors = sectorSize = 0;

	for (auto& i : _data)
	{
		auto& sector = i.second;
		if (sector)
		{
			numTracks = std::max(numTracks, (unsigned)sector->logicalTrack+1);
			numHeads = std::max(numHeads, (unsigned)sector->logicalSide+1);
			numSectors = std::max(numSectors, (unsigned)sector->logicalSector+1);
			sectorSize = std::max(sectorSize, (unsigned)sector->data.size());
		}
	}
}


