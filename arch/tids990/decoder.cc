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
#include <fmt/format.h>

/* The Texas Instruments DS990 uses MFM with a scheme similar to a simplified
 * version of the IBM record scheme (it's actually easier to parse than IBM).
 * There are 26 sectors per track, each holding a rather weird 288 bytes.
 */

/*
 * Sector record:
 * data:    0  1  0  1  0  1  0  1 .0  0  0  0  1  0  1  0  = 0x550a
 * mfm:     00 01 00 01 00 01 00 01.00 10 10 10 01 00 01 00 = 0x11112a44
 * special: 00 01 00 01 00 01 00 01.00 10 00 10 01 00 01 00 = 0x11112244
 *                                        ^^
 * When shifted out of phase, the special 0xa1 byte becomes an illegal
 * encoding (you can't do 10 00). So this can't be spoofed by user data.
 */
const FluxPattern SECTOR_RECORD_PATTERN(32, 0x11112244);

/*
 * Data record:
 * data:    0  1  0  1  0  1  0  1 .0  0  0  0  1  0  1  1  = 0x550c
 * mfm:     00 01 00 01 00 01 00 01.00 10 10 10 01 00 01 01 = 0x11112a45
 * special: 00 01 00 01 00 01 00 01.00 10 00 10 01 00 01 01 = 0x11112245
 *                                        ^^
 * When shifted out of phase, the special 0xa1 byte becomes an illegal
 * encoding (you can't do 10 00). So this can't be spoofed by user data.
 */
const FluxPattern DATA_RECORD_PATTERN(32, 0x11112245);

const FluxMatchers ANY_RECORD_PATTERN({ &SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN });

AbstractDecoder::RecordType TiDs990Decoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);
	if (matcher == &SECTOR_RECORD_PATTERN)
		return RecordType::SECTOR_RECORD;
	if (matcher == &DATA_RECORD_PATTERN)
		return RecordType::DATA_RECORD;
	return RecordType::UNKNOWN_RECORD;
}

void TiDs990Decoder::decodeSectorRecord()
{
    auto bits = readRawBits(TIDS990_SECTOR_RECORD_SIZE*16);
    auto bytes = decodeFmMfm(bits).slice(0, TIDS990_SECTOR_RECORD_SIZE);

    ByteReader br(bytes);
	uint16_t gotChecksum = crc16(CCITT_POLY, bytes.slice(1, TIDS990_SECTOR_RECORD_SIZE-3));

	br.seek(2);
	_sector->logicalSide = br.read_8() >> 3;
	_sector->logicalTrack = br.read_8();
	br.read_8(); /* number of sectors per track */
	_sector->logicalSector = br.read_8();
	br.read_be16(); /* sector size */
    uint16_t wantChecksum = br.read_be16();

    if (wantChecksum == gotChecksum)
        _sector->status = Sector::DATA_MISSING; /* correct but unintuitive */
}

void TiDs990Decoder::decodeDataRecord()
{
	auto bits = readRawBits(TIDS990_DATA_RECORD_SIZE*16);
	auto bytes = decodeFmMfm(bits).slice(0, TIDS990_DATA_RECORD_SIZE);

	ByteReader br(bytes);
	uint16_t gotChecksum = crc16(CCITT_POLY, bytes.slice(1, TIDS990_DATA_RECORD_SIZE-3));

	br.seek(2);
	_sector->data = br.read(TIDS990_PAYLOAD_SIZE);
	uint16_t wantChecksum = br.read_be16();
    _sector->status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}

