#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"

class Sector;
class Fluxmap;
class RawRecord;
class RawBits;

typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;
typedef std::vector<std::unique_ptr<Sector>> SectorVector;

extern void setDecoderManualClockRate(double clockrate_us);

extern Bytes decodeFmMfm(std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

static inline Bytes decodeFmMfm(const std::vector<bool> bits)
{ return decodeFmMfm(bits.begin(), bits.end()); }

class AbstractDecoder
{
public:
    virtual ~AbstractDecoder() {}

    nanoseconds_t guessClock(Fluxmap& fluxmap, unsigned physicalTrack) const;
    virtual nanoseconds_t guessClockImpl(Fluxmap& fluxmap, unsigned physicalTrack) const;

    virtual void decodeToSectors(const RawBits& bitmap, unsigned physicalTrack,
        RawRecordVector& rawrecords, SectorVector& sectors) = 0;
};

class AbstractSeparatedDecoder : public AbstractDecoder
{
public:
    virtual ~AbstractSeparatedDecoder() {}

    virtual RawRecordVector extractRecords(const RawBits& rawbits) const = 0;
    virtual SectorVector decodeToSectors(const RawRecordVector& rawrecords,
            unsigned physicalTrack) = 0;

    void decodeToSectors(const RawBits& bitmap, unsigned physicalTrack,
        RawRecordVector& rawrecords, SectorVector& sectors);
};

class AbstractSoftSectorDecoder : public AbstractSeparatedDecoder
{
public:
    virtual ~AbstractSoftSectorDecoder() {}

    RawRecordVector extractRecords(const RawBits& rawbits) const;

    virtual int recordMatcher(uint64_t fifo) const = 0;
};

class AbstractHardSectorDecoder : public AbstractSeparatedDecoder
{
public:
    virtual ~AbstractHardSectorDecoder() {}

    RawRecordVector extractRecords(const RawBits& rawbits) const;
};

class AbstractStatefulDecoder : public AbstractDecoder
{
};

#endif
