#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "decoders.h"
#include "record.h"
#include "protocol.h"
#include "rawbits.h"
#include "track.h"
#include "sector.h"
#include "fmt/format.h"
#include <numeric>

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
        Fluxmap::Position recordStart = sector.position = fmr.tell();
        sector.clock = 0;
        sector.status = Sector::MISSING;
        sector.data.clear();
        sector.logicalSector = sector.logicalSide = sector.logicalTrack = 0;
        RecordType r = advanceToNextRecord();
        if (fmr.eof() || !sector.clock)
            return;
        if ((r == UNKNOWN_RECORD) || (r == DATA_RECORD))
        {
            fmr.readNextMatchingOpcode(F_OP_PULSE);
            continue;
        }

        /* Read the sector record. */

        recordStart = fmr.tell();
        decodeSectorRecord();
        pushRecord(recordStart, fmr.tell());
        if (sector.status == Sector::DATA_MISSING)
        {
            /* The data is in a separate record. */

            r = advanceToNextRecord();
            if (r == DATA_RECORD)
            {
                recordStart = fmr.tell();
                decodeDataRecord();
                pushRecord(recordStart, fmr.tell());
            }
        }

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
