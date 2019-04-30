#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"
#include "sector.h"
#include "record.h"
#include "fluxmapreader.h"

class Sector;
class Fluxmap;
class FluxmapReader;
class RawRecord;
class RawBits;
class Track;

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

    nanoseconds_t guessClock(Track& track) const;
    virtual nanoseconds_t guessClockImpl(Track& track) const;

    virtual void decodeToSectors(Track& track) = 0;
};

class AbstractSimplifiedDecoder : public AbstractDecoder
{
public:
    enum RecordType
    {
        SECTOR_RECORD,
        DATA_RECORD,
        UNKNOWN_RECORD
    };

public:
    void decodeToSectors(Track& track) override;
    void pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end);

    std::vector<bool> readRawBits(unsigned count)
    { return _fmr->readRawBits(count, _sector->clock); }

    Fluxmap::Position tell()
    { return _fmr->tell(); } 

    void seek(const Fluxmap::Position& pos)
    { return _fmr->seek(pos); } 

protected:
    virtual RecordType advanceToNextRecord() = 0;
    virtual void decodeSectorRecord() = 0;
    virtual void decodeDataRecord() {};

    FluxmapReader* _fmr;
    Track* _track;
    Sector* _sector;
};

class AbstractStatefulDecoder : public AbstractDecoder
{
public:
    void decodeToSectors(Track& track);
    void discardRecord(FluxmapReader& fmr);
    void pushRecord(FluxmapReader& fmr, Track& track, Sector& sector);

protected:
    virtual nanoseconds_t findSector(FluxmapReader& fmr, Track& track) = 0;
    virtual void decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector) = 0;

private:
    Fluxmap::Position _recordStart;
};

class AbstractSplitDecoder : public AbstractStatefulDecoder
{
    void decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector) override;

    virtual nanoseconds_t findData(FluxmapReader& fmr, Track& track) = 0;
    virtual void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) = 0;
    virtual void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) = 0;
};

#endif
