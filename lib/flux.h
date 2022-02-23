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

struct TrackDataFlux
{
	unsigned physicalCylinder;
	unsigned physicalHead;
	std::shared_ptr<const Fluxmap> fluxmap;
	std::vector<std::shared_ptr<Record>> records;
	std::vector<std::shared_ptr<Sector>> sectors;
};

struct TrackFlux
{
	unsigned physicalCylinder;
	unsigned physicalHead;
	std::vector<std::shared_ptr<TrackDataFlux>> trackDatas;
	std::set<std::shared_ptr<Sector>> sectors;
};

struct DiskFlux
{
	std::vector<std::shared_ptr<TrackFlux>> tracks;
	std::shared_ptr<Image> image;
};

#endif

