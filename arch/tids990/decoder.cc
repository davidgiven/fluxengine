#include "globals.h"
#include "decoders/decoders.h"
#include "tids990/tids990.h"
#include "crc.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "sector.h"
#include "record.h"
#include "track.h"
#include <string.h>

const int SECTOR_SIZE = 256;

const FluxPattern ID_PATTERN(32, 0xaaaaffaf);

AbstractDecoder::RecordType TiDs990Decoder::advanceToNextRecord()
{
      return UNKNOWN_RECORD;
//    if (_currentSector == -1)
//    {
//        /* First sector in the track: look for the sync marker. */
//        const FluxMatcher* matcher = nullptr;
//        _sector->clock = _clock = _fmr->seekToPattern(ID_PATTERN, matcher);
//        readRawBits(32); /* skip the ID mark */
//        _logicalTrack = decodeFmMfm(readRawBits(32)).slice(0, 32).reader().read_be16();
//    }
//    else if (_currentSector == 10)
//    {
//        /* That was the last sector on the disk. */
//        return UNKNOWN_RECORD;
//    }
//    else
//    {
//        /* Otherwise we assume the clock from the first sector is still valid.
//         * The decoder framwork will automatically stop when we hit the end of
//         * the track. */
//        _sector->clock = _clock;
//    }
//
//    _currentSector++;
//    return SECTOR_RECORD;
}

void TiDs990Decoder::decodeSectorRecord()
{
//    auto bits = readRawBits((SECTOR_SIZE+2)*16);
//    auto bytes = decodeFmMfm(bits).slice(0, SECTOR_SIZE+2).swab();
//
//    uint16_t gotChecksum = 0;
//    ByteReader br(bytes);
//    for (int i=0; i<(SECTOR_SIZE/2); i++)
//        gotChecksum += br.read_le16();
//    uint16_t wantChecksum = br.read_le16();
//
//    _sector->logicalTrack = _logicalTrack;
//    _sector->logicalSide = _track->physicalSide;
//    _sector->logicalSector = _currentSector;
//    _sector->data = bytes.slice(0, SECTOR_SIZE);
//    _sector->status = (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}

