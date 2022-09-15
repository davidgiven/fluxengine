#ifndef FLUX_H
#define FLUX_H

#include "bytes.h"

class Fluxmap;
class Sector;
class Image;
class Track;

struct Record
{
	nanoseconds_t clock = 0;
	nanoseconds_t startTime = 0;
	nanoseconds_t endTime = 0;
	Bytes rawData;
};

struct TrackDataFlux
{
	std::shared_ptr<const Track> layout;
	std::shared_ptr<const Fluxmap> fluxmap;
	std::vector<std::shared_ptr<const Record>> records;
	std::vector<std::shared_ptr<const Sector>> sectors;
};

struct TrackFlux
{
	std::shared_ptr<const Track> layout;
	std::vector<std::shared_ptr<TrackDataFlux>> trackDatas;
	std::set<std::shared_ptr<const Sector>> sectors;
};

struct DiskFlux
{
	std::vector<std::shared_ptr<TrackFlux>> tracks;
	std::shared_ptr<const Image> image;
};

#endif

