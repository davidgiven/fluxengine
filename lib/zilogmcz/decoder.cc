#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "zilogmcz.h"
#include "bytes.h"
#include "crc.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

static const FluxPattern SECTOR_START_PATTERN(16, 0xaaab);

AbstractDecoder::RecordType ZilogMczDecoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
    _fmr->seekToIndexMark();
	_sector->clock = _fmr->seekToPattern(SECTOR_START_PATTERN, matcher);
	if (matcher == &SECTOR_START_PATTERN)
		return SECTOR_RECORD;
	return UNKNOWN_RECORD;
}

void ZilogMczDecoder::decodeSectorRecord()
{
    readRawBits(14);

    auto rawbits = readRawBits(140*16);
    auto bytes = decodeFmMfm(rawbits).slice(0, 140);
    ByteReader br(bytes);

    _sector->logicalSector = br.read_8() & 0x1f;
    _sector->logicalSide = 0;
    _sector->logicalTrack = br.read_8() & 0x7f;
    if (_sector->logicalSector > 31)
        return;
    if (_sector->logicalTrack > 80)
        return;

    _sector->data = br.read(132);
    uint16_t wantChecksum = br.read_be16();
    uint16_t gotChecksum = crc16(MODBUS_POLY, 0x0000, bytes.slice(0, 134));

    _sector->status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
