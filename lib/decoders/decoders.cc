#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/config/config.h"
#include "lib/decoders/decoders.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/flux.h"
#include "protocol.h"
#include "lib/decoders/rawbits.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/data/layout.h"
#include <numeric>

std::shared_ptr<TrackDataFlux> Decoder::decodeToSectors(
    std::shared_ptr<const Fluxmap> fluxmap,
    std::shared_ptr<const TrackInfo>& trackInfo)
{
    _trackdata = std::make_shared<TrackDataFlux>();
    _trackdata->fluxmap = fluxmap;
    _trackdata->trackInfo = trackInfo;

    FluxmapReader fmr(*fluxmap);
    _fmr = &fmr;

    auto newSector = [&]
    {
        _sector = std::make_shared<Sector>(trackInfo, 0);
        _sector->status = Sector::MISSING;
    };

    newSector();
    beginTrack();
    for (;;)
    {
        newSector();

        Fluxmap::Position recordStart = fmr.tell();
        _sector->clock = advanceToNextRecord();
        if (fmr.eof() || !_sector->clock)
            break;

        /* Read the sector record. */

        Fluxmap::Position before = fmr.tell();
        decodeSectorRecord();
        Fluxmap::Position after = fmr.tell();
        pushRecord(before, after);

        if (_sector->status != Sector::DATA_MISSING)
        {
            _sector->position = before.bytes;
            _sector->dataStartTime = before.ns();
            _sector->dataEndTime = after.ns();
        }
        else
        {
            /* The data is in a separate record. */

            _sector->headerStartTime = before.ns();
            _sector->headerEndTime = after.ns();

            _sector->clock = advanceToNextRecord();
            if (fmr.eof() || !_sector->clock)
                break;

            before = fmr.tell();
            decodeDataRecord();
            after = fmr.tell();

            if (_sector->status != Sector::DATA_MISSING)
            {
                _sector->position = before.bytes;
                _sector->dataStartTime = before.ns();
                _sector->dataEndTime = after.ns();
                pushRecord(before, after);
            }
            else
            {
                fmr.skipToEvent(F_BIT_PULSE);
                resetFluxDecoder();
            }
        }

        if (_sector->status != Sector::MISSING)
        {
            auto trackLayout = Layout::getLayoutOfTrack(
                _sector->logicalTrack, _sector->logicalSide);
            _trackdata->sectors.push_back(_sector);
        }
    }

    return _trackdata;
}

void Decoder::pushRecord(
    const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

    auto record = std::make_shared<Record>();
    _trackdata->records.push_back(record);
    _sector->records.push_back(record);

    record->position = start.bytes;
    record->startTime = start.ns();
    record->endTime = end.ns();
    record->clock = _sector->clock;

    record->rawData = toBytes(_recordBits);
    _recordBits.clear();
}

void Decoder::resetFluxDecoder()
{
    _decoder.reset(new FluxDecoder(_fmr, _sector->clock, _config));
}

nanoseconds_t Decoder::seekToPattern(const FluxMatcher& pattern)
{
    nanoseconds_t clock = _fmr->seekToPattern(pattern);
    _decoder.reset(new FluxDecoder(_fmr, clock, _config));
    return clock;
}

void Decoder::seekToIndexMark()
{
    _fmr->skipToEvent(F_BIT_PULSE);
    _fmr->seekToIndexMark();
}

std::vector<bool> Decoder::readRawBits(unsigned count)
{
    auto bits = _decoder->readBits(count);
    _recordBits.insert(_recordBits.end(), bits.begin(), bits.end());
    return bits;
}

uint8_t Decoder::readRaw8()
{
    return toBytes(readRawBits(8)).reader().read_8();
}

uint16_t Decoder::readRaw16()
{
    return toBytes(readRawBits(16)).reader().read_be16();
}

uint32_t Decoder::readRaw20()
{
    std::vector<bool> bits(4);
    for (bool b : readRawBits(20))
        bits.push_back(b);

    return toBytes(bits).reader().read_be24();
}

uint32_t Decoder::readRaw24()
{
    return toBytes(readRawBits(24)).reader().read_be24();
}

uint32_t Decoder::readRaw32()
{
    return toBytes(readRawBits(32)).reader().read_be32();
}

uint64_t Decoder::readRaw48()
{
    return toBytes(readRawBits(48)).reader().read_be48();
}

uint64_t Decoder::readRaw64()
{
    return toBytes(readRawBits(64)).reader().read_be64();
}
