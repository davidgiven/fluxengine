#include "globals.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"

std::unique_ptr<Sector>& SectorSet::operator[](const key_t& key)
{
	return _data[key];
}

Sector* SectorSet::operator[](const key_t& key) const
{
	auto i = _data.find(key);
	if (i == _data.end())
		return NULL;
	return i->second.get();
}

void SectorSet::calculateSize(int& numTracks, int& numHeads, int& numSectors,
	int& sectorSize) const
{
	numTracks = numHeads = numSectors = sectorSize = 0;

	for (auto& i : _data)
	{
		auto& sector = i.second;
		if (sector)
		{
			numTracks = std::max(numTracks, sector->track+1);
			numHeads = std::max(numHeads, sector->side+1);
			numSectors = std::max(numSectors, sector->sector+1);
			sectorSize = std::max(sectorSize, (int)sector->data.size());
		}
	}
}


