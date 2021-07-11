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
#include "record.h"
#include "protocol.h"
#include "decoders/rawbits.h"
#include "track.h"
#include "sector.h"
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
		#if 0
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
		#endif
	};

	auto decoder = decoders.find(config.format_case());
	if (decoder == decoders.end())
		Error() << "no decoder specified";

	return (decoder->second)(config);
}

void AbstractDecoder::decodeToSectors(FluxTrackProto& track)
{
	SectorProto sector;
	_sector = &sector;

    _sector->set_physical_head(track.physical_head());
    _sector->set_physical_cylinder(track.physical_cylinder());
	Fluxmap fm(track.flux());
    FluxmapReader fmr(fm);

    _track = &track;
    _fmr = &fmr;

    beginTrack();
    for (;;)
    {
        Fluxmap::Position recordStart = fmr.tell();
		_sector->Clear();
        _sector->set_status(SectorStatus::MISSING);
        RecordType r = advanceToNextRecord();
        if (fmr.eof() || !sector.clock())
            return;
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
        if (sector.status() == SectorStatus::DATA_MISSING)
        {
            /* The data is in a separate record. */

            sector.set_header_starttime_ns(recordStart.ns());
            sector.set_header_endtime_ns(recordEnd.ns());
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
        sector.set_data_starttime_ns(recordStart.ns());
        sector.set_data_endtime_ns(recordEnd.ns());

        if (sector.status() != SectorStatus::MISSING)
			*(_track->add_sector()) = sector;
    }
}

void AbstractDecoder::pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

	FluxRecordProto* record = _track->add_record();
	record->set_record_starttime_ns(start.ns());
	record->set_record_endtime_ns(end.ns());
    record->set_physical_head(_track->physical_head());
    record->set_physical_cylinder(_track->physical_cylinder());
    record->set_clock(_sector->clock());

    _fmr->seek(start);
    record->set_data(toBytes(_fmr->readRawBits(end, _sector->clock())));
    _fmr->seek(here);
}

std::vector<bool> AbstractDecoder::readRawBits(unsigned count)
{
	return _fmr->readRawBits(count, _sector->clock());
}

std::set<unsigned> AbstractDecoder::requiredSectors(FluxTrackProto& track) const
{
	static std::set<unsigned> set;
	return set;
}

