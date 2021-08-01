#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
//#include "arch/aeslanier/aeslanier.h"
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
#include "lib/data.pb.h"
#include "fmt/format.h"
#include <numeric>

std::unique_ptr<AbstractDecoder> AbstractDecoder::create(const DecoderProto& config)
{
	static const std::map<int,
		std::function<std::unique_ptr<AbstractDecoder>(const DecoderProto&)>> decoders =
	{
		//{ DecoderProto::kAeslanier,  createAesLanierDecoder },
		{ DecoderProto::kAmiga,      createAmigaDecoder },
		{ DecoderProto::kApple2,     createApple2Decoder },
		{ DecoderProto::kBrother,    createBrotherDecoder },
		{ DecoderProto::kC64,        createCommodore64Decoder },
		{ DecoderProto::kF85,        createDurangoF85Decoder },
		{ DecoderProto::kFb100,      createFb100Decoder },
		{ DecoderProto::kIbm,        createIbmDecoder },
		{ DecoderProto::kMacintosh,  createMacintoshDecoder },
		{ DecoderProto::kMicropolis, createMicropolisDecoder },
		{ DecoderProto::kMx,         createMxDecoder },
		{ DecoderProto::kNorthstar,  createNorthstarDecoder },
		{ DecoderProto::kTids990,    createTids990Decoder },
		{ DecoderProto::kVictor9K,   createVictor9kDecoder },
		{ DecoderProto::kZilogmcz,   createZilogMczDecoder },
	};

	auto decoder = decoders.find(config.format_case());
	if (decoder == decoders.end())
		Error() << "no decoder specified";

	return (decoder->second)(config);
}

std::unique_ptr<TrackDataFlux> AbstractDecoder::decodeToSectors(
		std::shared_ptr<const Fluxmap> fluxmap, unsigned physicalCylinder, unsigned physicalHead)
{
	_trackdata = std::make_unique<TrackDataFlux>();
	_trackdata->fluxmap = fluxmap;
	_trackdata->physicalCylinder = physicalCylinder;
	_trackdata->physicalHead = physicalHead;
	
    FluxmapReader fmr(*fluxmap);
    _fmr = &fmr;

    beginTrack();
    for (;;)
    {
		_sector = std::make_shared<Sector>();
		_sector->status = Sector::MISSING;
		_sector->physicalCylinder = physicalCylinder;
		_sector->physicalHead = physicalHead;

        Fluxmap::Position recordStart = fmr.tell();
        RecordType r = advanceToNextRecord();
        if (fmr.eof() || !_sector->clock)
            return std::move(_trackdata);
        if ((r == UNKNOWN_RECORD) || (r == DATA_RECORD))
        {
            fmr.findEvent(F_BIT_PULSE);
            continue;
        }

        /* Read the sector record. */

        recordStart = fmr.tell();
        decodeSectorRecord();
        Fluxmap::Position recordEnd = fmr.tell();
        pushRecord(recordStart, recordEnd);
        if (_sector->status == Sector::DATA_MISSING)
        {
            /* The data is in a separate record. */

            _sector->headerStartTime = recordStart.ns();
            _sector->headerEndTime = recordEnd.ns();
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
        _sector->dataStartTime = recordStart.ns();
        _sector->dataEndTime = recordEnd.ns();

        if (_sector->status != Sector::MISSING)
			_trackdata->sectors.push_back(_sector);
    }
}

void AbstractDecoder::pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

	auto record = std::make_shared<Record>();
	_trackdata->records.push_back(record);
	
	record->startTime = start.ns();
	record->endTime = end.ns();
    record->clock = _sector->clock;

    _fmr->seek(start);
    record->rawData = toBytes(_fmr->readRawBits(end, _sector->clock));
    _fmr->seek(here);
}

std::vector<bool> AbstractDecoder::readRawBits(unsigned count)
{
	return _fmr->readRawBits(count, _sector->clock);
}

std::set<unsigned> AbstractDecoder::requiredSectors(unsigned cylinder, unsigned head) const
{
	static std::set<unsigned> set;
	return set;
}

