#ifndef FLUX_H
#define FLUX_H

#include "lib/bytes.h"

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
    unsigned logicalTrack;
	unsigned head;
	unsigned groupSize;

	bool operator==(const Location& other) const
	{
		if (physicalTrack == other.physicalTrack)
			return true;
		return head == other.head;
	}

    bool operator<(const Location& other) const
    {
		if (physicalTrack < other.physicalTrack)
			return true;
		if (physicalTrack == other.physicalTrack)
			return head < other.head;
		return false;
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

