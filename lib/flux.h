#ifndef FLUX_H
#define FLUX_H

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

    std::strong_ordering operator<=>(const Location& other) const
    {
		auto i = physicalTrack <=> other.physicalTrack;
		if (i == std::strong_ordering::equal)
			i = head <=> other.head;
		return i;
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

