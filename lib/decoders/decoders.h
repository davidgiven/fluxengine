#ifndef DECODERS_H
#define DECODERS_H

#include "segmenter.h"


class Sector;
class Fluxmap;
class RawRecord;
typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;
typedef std::vector<std::unique_ptr<Sector>> SectorVector;

extern std::vector<uint8_t> decodeFmMfm(const std::vector<bool> bits);

class AbstractDecoder : public AbstractSegmenter
{
public:
    virtual ~AbstractDecoder() {}

    virtual nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    virtual SectorVector decodeToSectors(const RawRecordVector& rawrecords,
            unsigned physicalTrack) = 0;
};

#endif
