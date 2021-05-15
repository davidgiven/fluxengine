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
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/micropolis/micropolis.h"
#include "arch/mx/mx.h"
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
	switch (config.format_case())
	{
		case DecoderProto::kAeslanier:
			return std::unique_ptr<AbstractDecoder>(new AesLanierDecoder(config.aeslanier()));

		case DecoderProto::kAmiga:
			return std::unique_ptr<AbstractDecoder>(new AmigaDecoder(config.amiga()));

		case DecoderProto::kApple2:
			return std::unique_ptr<AbstractDecoder>(new Apple2Decoder(config.apple2()));

		case DecoderProto::kBrother:
			return std::unique_ptr<AbstractDecoder>(new BrotherDecoder(config.brother()));

		case DecoderProto::kC64:
			return std::unique_ptr<AbstractDecoder>(new Commodore64Decoder(config.c64()));

		case DecoderProto::kF85:
			return std::unique_ptr<AbstractDecoder>(new DurangoF85Decoder(config.f85()));

		case DecoderProto::kIbm:
			return std::unique_ptr<AbstractDecoder>(new IbmDecoder(config.ibm()));

		case DecoderProto::kMacintosh:
			return std::unique_ptr<AbstractDecoder>(new MacintoshDecoder(config.macintosh()));

		case DecoderProto::kMicropolis:
			return std::unique_ptr<AbstractDecoder>(new MicropolisDecoder(config.micropolis()));

		case DecoderProto::kMx:
			return std::unique_ptr<AbstractDecoder>(new MxDecoder(config.mx()));

		default:
			Error() << "no input disk format specified";
	}

	return std::unique_ptr<AbstractDecoder>();
}

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

