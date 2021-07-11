#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"
#include "sector.h"
#include "record.h"
#include "decoders/fluxmapreader.h"

class Sector;
class Fluxmap;
class FluxmapReader;
class RawRecord;
class RawBits;
class Track;
class DecoderProto;

typedef std::vector<std::unique_ptr<RawRecord>> RawRecordVector;
typedef std::vector<std::unique_ptr<Sector>> SectorVector;

extern void setDecoderManualClockRate(double clockrate_us);

extern Bytes decodeFmMfm(std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);
extern void encodeMfm(std::vector<bool>& bits, unsigned& cursor, const Bytes& input, bool& lastBit);
extern void encodeFm(std::vector<bool>& bits, unsigned& cursor, const Bytes& input);
extern Bytes encodeMfm(const Bytes& input, bool& lastBit);

static inline Bytes decodeFmMfm(const std::vector<bool> bits)
{ return decodeFmMfm(bits.begin(), bits.end()); }

class AbstractDecoder
{
public:
	AbstractDecoder() {} // REMOVE ME
	AbstractDecoder(const DecoderProto& config) {}

    virtual ~AbstractDecoder() {}

	static std::unique_ptr<AbstractDecoder> create(const DecoderProto& config);

public:
    enum RecordType
    {
        SECTOR_RECORD,
        DATA_RECORD,
        UNKNOWN_RECORD
    };

public:
    void decodeToSectors(Track& track);
    void pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end);

    std::vector<bool> readRawBits(unsigned count)
    { return _fmr->readRawBits(count, _sector->clock); }

    Fluxmap::Position tell()
    { return _fmr->tell(); } 

    void seek(const Fluxmap::Position& pos)
    { return _fmr->seek(pos); } 

	/* Returns a set of sectors required to exist on this track. If the reader
	 * sees any missing, it will consider this to be an error and will retry
	 * the read. */
	virtual std::set<unsigned> requiredSectors(Track& track) const;

protected:
    virtual void beginTrack() {};
    virtual RecordType advanceToNextRecord() = 0;
    virtual void decodeSectorRecord() = 0;
    virtual void decodeDataRecord() {};

    FluxmapReader* _fmr;
    Track* _track;
    Sector* _sector;
};

#endif
