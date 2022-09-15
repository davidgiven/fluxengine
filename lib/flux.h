#ifndef FLUX_H
#define FLUX_H

#include "bytes.h"

class Fluxmap;
class Sector;
class Image;

struct Record
{
	nanoseconds_t clock = 0;
	nanoseconds_t startTime = 0;
	nanoseconds_t endTime = 0;
	Bytes rawData;
};

struct Location
{
    unsigned physicalTrack;
    unsigned physicalSide;
    unsigned logicalTrack;
	unsigned logicalSide;
	unsigned groupSize;

	bool operator==(const Location& other) const
	{
		return key() == other.key();
	}

    bool operator<(const Location& other) const
    {
		return key() < other.key();
    }

private:
    std::tuple<unsigned, unsigned> key() const
    {
    	return std::make_tuple(physicalTrack, physicalSide);
    }
};

struct TrackDataFlux
{
	Location location;
	std::shared_ptr<const Fluxmap> fluxmap;
	std::vector<std::shared_ptr<const Record>> records;
	std::vector<std::shared_ptr<const Sector>> sectors;
};

struct TrackFlux
{
	Location location;
	std::vector<std::shared_ptr<const TrackDataFlux>> trackDatas;
	std::set<std::shared_ptr<const Sector>> sectors;
};

struct DiskFlux
{
	std::vector<std::shared_ptr<const TrackFlux>> tracks;
	std::shared_ptr<const Image> image;
};

#endif

