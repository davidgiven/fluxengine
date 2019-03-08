#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"

class Sector;
class Fluxmap;
class RawRecord;
typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;
typedef std::vector<std::unique_ptr<Sector>> SectorVector;

extern Bytes decodeFmMfm(const std::vector<bool> bits);

class AbstractDecoder
{
public:
    virtual ~AbstractDecoder() {}

    virtual nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    virtual RawRecordVector extractRecords(std::vector<bool> bits) const = 0;
    virtual SectorVector decodeToSectors(const RawRecordVector& rawrecords,
            unsigned physicalTrack) = 0;
};

class AbstractSoftSectorDecoder : public AbstractDecoder
{
public:
    virtual ~AbstractSoftSectorDecoder() {}

    RawRecordVector extractRecords(std::vector<bool> bits) const;

    virtual int recordMatcher(uint64_t fifo) const = 0;
};

#endif
