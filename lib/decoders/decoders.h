#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"

class Sector;
class Fluxmap;
class RawRecord;
class RawBits;

typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;
typedef std::vector<std::unique_ptr<Sector>> SectorVector;

extern Bytes decodeFmMfm(const std::vector<bool> bits);

class AbstractDecoder
{
public:
    virtual ~AbstractDecoder() {}

    virtual nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    virtual RawRecordVector extractRecords(const RawBits& rawbits) const = 0;
    virtual SectorVector decodeToSectors(const RawRecordVector& rawrecords,
            unsigned physicalTrack) = 0;
};

class AbstractSoftSectorDecoder : public AbstractDecoder
{
public:
    virtual ~AbstractSoftSectorDecoder() {}

    RawRecordVector extractRecords(const RawBits& rawbits) const;

    virtual int recordMatcher(uint64_t fifo) const = 0;
};

class AbstractHardSectorDecoder : public AbstractDecoder
{
public:
    virtual ~AbstractHardSectorDecoder() {}

    RawRecordVector extractRecords(const RawBits& bits) const;
};

#endif
