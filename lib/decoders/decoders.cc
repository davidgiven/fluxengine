#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
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
#include "record.h"
#include "protocol.h"
#include "decoders/rawbits.h"
#include "track.h"
#include "sector.h"
#include "lib/decoders/decoders.pb.h"
#include "fmt/format.h"
#include <numeric>

std::unique_ptr<AbstractDecoder> AbstractDecoder::create(const DecoderProto& config)
{
	static const std::map<int,
		std::function<std::unique_ptr<AbstractDecoder>(const DecoderProto&)>> decoders =
	{
		{ DecoderProto::kAeslanier, createAesLanierDecoder },
		{ DecoderProto::kAmiga,     createAmigaDecoder },
		{ DecoderProto::kApple2,    createApple2Decoder },
		{ DecoderProto::kBrother,   createBrotherDecoder },
		{ DecoderProto::kC64,       createCommodore64Decoder },
		{ DecoderProto::kF85,       createDurangoF85Decoder },
		{ DecoderProto::kFb100,     createFb100Decoder },
		{ DecoderProto::kIbm,       createIbmDecoder },
		{ DecoderProto::kMacintosh, createMacintoshDecoder },
	};

	auto decoder = decoders.find(config.format_case());
	if (decoder == decoders.end())
		Error() << "no decoder specified";

	return (decoder->second)(config);
}

#if 0
std::unique_ptr<AbstractDecoder> AbstractDecoder::create(const DecoderProto& config)
{
	switch (config.format_case())
	{
		case DecoderProto::kMicropolis:
			return std::unique_ptr<AbstractDecoder>(new MicropolisDecoder(config.micropolis()));

		case DecoderProto::kMx:
			return std::unique_ptr<AbstractDecoder>(new MxDecoder(config.mx()));

		case DecoderProto::kTids990:
			return std::unique_ptr<AbstractDecoder>(new Tids990Decoder(config.tids990()));

		case DecoderProto::kVictor9K:
			return std::unique_ptr<AbstractDecoder>(new Victor9kDecoder(config.victor9k()));

		case DecoderProto::kZilogmcz:
			return std::unique_ptr<AbstractDecoder>(new ZilogMczDecoder(config.zilogmcz()));

		case DecoderProto::kNorthstar:
			return std::unique_ptr<AbstractDecoder>(new NorthstarDecoder(config.northstar()));

		default:
			Error() << "no input disk format specified";
	}

	return std::unique_ptr<AbstractDecoder>();
}
#endif

void AbstractDecoder::decodeToSectors(Track& track)
{
    Sector sector;
    sector.physicalSide = track.physicalSide;
    sector.physicalTrack = track.physicalTrack;
    FluxmapReader fmr(*track.fluxmap);

    _track = &track;
    _sector = &sector;
    _fmr = &fmr;

    beginTrack();
    for (;;)
    {
        Fluxmap::Position recordStart = fmr.tell();
        sector.clock = 0;
        sector.status = Sector::MISSING;
        sector.data.clear();
        sector.logicalSector = sector.logicalSide = sector.logicalTrack = 0;
        RecordType r = advanceToNextRecord();
        if (fmr.eof() || !sector.clock)
            return;
        if ((r == UNKNOWN_RECORD) || (r == DATA_RECORD))
        {
            fmr.findEvent(F_BIT_PULSE);
            continue;
        }

        /* Read the sector record. */

        sector.position = recordStart = fmr.tell();
        decodeSectorRecord();
        Fluxmap::Position recordEnd = fmr.tell();
        pushRecord(recordStart, recordEnd);
        if (sector.status == Sector::DATA_MISSING)
        {
            /* The data is in a separate record. */

            sector.headerStartTime = recordStart.ns();
            sector.headerEndTime = recordEnd.ns();
			for (;;)
			{
				r = advanceToNextRecord();
				if (r != UNKNOWN_RECORD)
					break;
				if (fmr.findEvent(F_BIT_PULSE) == 0)
                    break;
			}
            recordStart = fmr.tell();
            if (r == DATA_RECORD)
                decodeDataRecord();
            recordEnd = fmr.tell();
            pushRecord(recordStart, recordEnd);
        }
        sector.dataStartTime = recordStart.ns();
        sector.dataEndTime = recordEnd.ns();

        if (sector.status != Sector::MISSING)
            track.sectors.push_back(sector);
    }
}

void AbstractDecoder::pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

    RawRecord record;
    record.physicalSide = _track->physicalSide;
    record.physicalTrack = _track->physicalTrack;
    record.clock = _sector->clock;
    record.position = start;

    _fmr->seek(start);
    record.data = toBytes(_fmr->readRawBits(end, _sector->clock));
    _track->rawrecords.push_back(record);
    _fmr->seek(here);
}

std::set<unsigned> AbstractDecoder::requiredSectors(Track& track) const
{
	static std::set<unsigned> empty;
	return empty;
}

