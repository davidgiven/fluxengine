#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "arch/agat/agat.h"
#include "arch/aeslanier/aeslanier.h"
#include "arch/amiga/amiga.h"
#include "arch/apple2/apple2.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/f85/f85.h"
#include "arch/fb100/fb100.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/micropolis/micropolis.h"
#include "arch/mx/mx.h"
#include "arch/northstar/northstar.h"
#include "arch/tids990/tids990.h"
#include "arch/victor9k/victor9k.h"
#include "arch/zilogmcz/zilogmcz.h"
#include "decoders/fluxmapreader.h"
#include "flux.h"
#include "protocol.h"
#include "decoders/rawbits.h"
#include "sector.h"
#include "image.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/layout.h"
#include "fmt/format.h"
#include <numeric>

std::unique_ptr<Decoder> Decoder::create(const DecoderProto& config)
{
    static const std::map<int,
        std::function<std::unique_ptr<Decoder>(const DecoderProto&)>>
        decoders = {
            {DecoderProto::kAgat,       createAgatDecoder       },
            {DecoderProto::kAeslanier,  createAesLanierDecoder  },
            {DecoderProto::kAmiga,      createAmigaDecoder      },
            {DecoderProto::kApple2,     createApple2Decoder     },
            {DecoderProto::kBrother,    createBrotherDecoder    },
            {DecoderProto::kC64,        createCommodore64Decoder},
            {DecoderProto::kF85,        createDurangoF85Decoder },
            {DecoderProto::kFb100,      createFb100Decoder      },
            {DecoderProto::kIbm,        createIbmDecoder        },
            {DecoderProto::kMacintosh,  createMacintoshDecoder  },
            {DecoderProto::kMicropolis, createMicropolisDecoder },
            {DecoderProto::kMx,         createMxDecoder         },
            {DecoderProto::kNorthstar,  createNorthstarDecoder  },
            {DecoderProto::kTids990,    createTids990Decoder    },
            {DecoderProto::kVictor9K,   createVictor9kDecoder   },
            {DecoderProto::kZilogmcz,   createZilogMczDecoder   },
    };

    auto decoder = decoders.find(config.format_case());
    if (decoder == decoders.end())
        Error() << "no decoder specified";

    return (decoder->second)(config);
}

std::shared_ptr<const TrackDataFlux> Decoder::decodeToSectors(
    std::shared_ptr<const Fluxmap> fluxmap, const Location& location)
{
    _trackdata = std::make_shared<TrackDataFlux>();
    _trackdata->fluxmap = fluxmap;
    _trackdata->location = location;

    FluxmapReader fmr(*fluxmap);
    _fmr = &fmr;

    auto newSector = [&]
    {
        _sector = std::make_shared<Sector>(location);
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
            return _trackdata;

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

            for (;;)
            {
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
                    break;
                }

                fmr.skipToEvent(F_BIT_PULSE);
                resetFluxDecoder();
            }
        }

        if (_sector->status != Sector::MISSING)
        {
            auto& trackLayout = Layout::getLayoutOfTrack(
                _sector->logicalTrack, _sector->logicalSide);
            _trackdata->sectors.push_back(_sector);
        }
    }
}

void Decoder::pushRecord(
    const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

    auto record = std::make_shared<Record>();
    _trackdata->records.push_back(record);
    _sector->records.push_back(record);

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

std::set<LogicalLocation> Decoder::requiredSectors(
    const Location& location) const
{
    const auto& trackLayout =
        Layout::getLayoutOfTrackPhysical(location.physicalTrack, location.physicalSide);

    std::set<LogicalLocation> results;
    for (unsigned sectorId : trackLayout.logicalSectorOrder)
        results.insert(LogicalLocation{
            trackLayout.logicalTrack, trackLayout.logicalSide, sectorId});
    return results;
}
