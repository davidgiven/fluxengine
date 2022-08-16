#ifndef DECODERS_H
#define DECODERS_H

#include "bytes.h"
#include "sector.h"
#include "decoders/fluxmapreader.h"
#include "decoders/fluxdecoder.h"

class Sector;
class Fluxmap;
class FluxmapReader;
class RawBits;
class DecoderProto;

#include "flux.h"

extern void setDecoderManualClockRate(double clockrate_us);

extern Bytes decodeFmMfm(std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);
extern void encodeMfm(std::vector<bool>& bits,
    unsigned& cursor,
    const Bytes& input,
    bool& lastBit);
extern void encodeFm(
    std::vector<bool>& bits, unsigned& cursor, const Bytes& input);
extern Bytes encodeMfm(const Bytes& input, bool& lastBit);

static inline Bytes decodeFmMfm(const std::vector<bool> bits)
{
    return decodeFmMfm(bits.begin(), bits.end());
}

class AbstractDecoder
{
public:
    AbstractDecoder(const DecoderProto& config): _config(config) {}

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
    std::shared_ptr<const TrackDataFlux> decodeToSectors(
        std::shared_ptr<const Fluxmap> fluxmap,
		const Location& location);

    void pushRecord(
        const Fluxmap::Position& start, const Fluxmap::Position& end);

    void resetFluxDecoder();
    std::vector<bool> readRawBits(unsigned count);
    uint8_t readRaw8();
    uint16_t readRaw16();
    uint32_t readRaw20();
    uint32_t readRaw24();
    uint32_t readRaw32();
    uint64_t readRaw48();
    uint64_t readRaw64();

    Fluxmap::Position tell()
    {
        return _fmr->tell();
    }

    void seek(const Fluxmap::Position& pos)
    {
        return _fmr->seek(pos);
    }

    nanoseconds_t seekToPattern(const FluxMatcher& pattern);
    void seekToIndexMark();

    bool eof() const
    {
        return _fmr->eof();
    }

    nanoseconds_t getFluxmapDuration() const
    {
        return _fmr->getDuration();
    }

    virtual std::set<unsigned> requiredSectors(const Location& location) const;

    /* Indicates whether records have a unique start pattern, e.g. whether
     * must be read contiguously (Amiga) or can be search for from any
     * position (IBM). */
    virtual bool recordsSeekable() const
    {
        return true;
    }

protected:
    virtual void beginTrack(){};
    virtual nanoseconds_t advanceToNextRecord() = 0;
    virtual void decodeSectorRecord() = 0;
    virtual void decodeDataRecord(){};

    const DecoderProto& _config;
    std::shared_ptr<TrackDataFlux> _trackdata;
    std::shared_ptr<Sector> _sector;
    std::unique_ptr<FluxDecoder> _decoder;
    std::vector<bool> _recordBits;

private:
    FluxmapReader* _fmr = nullptr;
};

#endif
