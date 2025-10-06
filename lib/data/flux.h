#ifndef FLUX_H
#define FLUX_H

#include "lib/core/bytes.h"
#include "lib/data/locations.h"

class Fluxmap;
class Sector;
class Image;
class TrackInfo;
class DiskLayout;

struct Record
{
    nanoseconds_t clock = 0;
    nanoseconds_t startTime = 0;
    nanoseconds_t endTime = 0;
    uint32_t position = 0;
    Bytes rawData;
};

struct DecodedTrack
{
    std::shared_ptr<const TrackInfo> trackInfo;
    std::shared_ptr<const Fluxmap> fluxmap;
    std::vector<std::shared_ptr<const Record>> records;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

struct DecodedDisk
{
    DecodedDisk& operator=(const DecodedDisk& other) = default;

    std::multimap<CylinderHead, std::shared_ptr<const DecodedTrack>>
        decodedTracks;
    std::multimap<CylinderHead, std::shared_ptr<const Sector>> sectorsByTrack;
    std::shared_ptr<const Image> image;
    std::shared_ptr<const DiskLayout> layout;
};

#endif
