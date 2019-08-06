#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "record.h"
#include "brother.h"
#include "sector.h"
#include "bytes.h"
#include "crc.h"
#include <ctype.h>

const FluxPattern SECTOR_RECORD_PATTERN(32, BROTHER_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(32, BROTHER_DATA_RECORD);
const FluxMatchers ANY_RECORD_PATTERN({ &SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN });

static std::vector<uint8_t> outputbuffer;

/*
 * Brother disks have this very very non-IBM system where sector header records
 * and data records use two different kinds of GCR: sector headers are 8-in-16
 * (but the encodable values range from 0 to 77ish only) and data headers are
 * 5-in-8. In addition, there's a non-encoded 10-bit ID word at the beginning
 * of each record, as well as a string of 53 1s introducing them. That does at
 * least make them easy to find.
 *
 * Disk formats vary from machine to machine, but mine uses 78 tracks. Track 0
 * is erased but not formatted.  Track alignment is extremely dubious and
 * Brother track 0 shows up on my machine at track 2.
 */

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static int decode_header_gcr(uint16_t word)
{
	switch (word)
	{
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "header_gcr.h"
		#undef GCR_ENTRY
	}                       
	return -1;             
};

AbstractDecoder::RecordType BrotherDecoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);
	if (matcher == &SECTOR_RECORD_PATTERN)
		return RecordType::SECTOR_RECORD;
	if (matcher == &DATA_RECORD_PATTERN)
		return RecordType::DATA_RECORD;
	return RecordType::UNKNOWN_RECORD;
}

void BrotherDecoder::decodeSectorRecord()
{
	readRawBits(32);
	const auto& rawbits = readRawBits(32);
	const auto& bytes = toBytes(rawbits).slice(0, 4);

	ByteReader br(bytes);
	_sector->logicalTrack = decode_header_gcr(br.read_be16());
	_sector->logicalSector = decode_header_gcr(br.read_be16());

	/* Sanity check the values read; there's no header checksum and
		* occasionally we get garbage due to bit errors. */
	if (_sector->logicalSector > 11)
		return;
	if (_sector->logicalTrack > 79)
		return;

	_sector->status = Sector::DATA_MISSING;
}

void BrotherDecoder::decodeDataRecord()
{
	readRawBits(32);

	const auto& rawbits = readRawBits(BROTHER_DATA_RECORD_ENCODED_SIZE*8);
	const auto& rawbytes = toBytes(rawbits).slice(0, BROTHER_DATA_RECORD_ENCODED_SIZE);

	Bytes bytes;
	ByteWriter bw(bytes);
	BitWriter bitw(bw);
	for (uint8_t b : rawbytes)
	{
		uint32_t nibble = decode_data_gcr(b);
		bitw.push(nibble, 5);
	}
	bitw.flush();

	_sector->data = bytes.slice(0, BROTHER_DATA_RECORD_PAYLOAD);
	uint32_t realCrc = crcbrother(_sector->data);
	uint32_t wantCrc = bytes.reader().seek(BROTHER_DATA_RECORD_PAYLOAD).read_be24();
	_sector->status = (realCrc == wantCrc) ? Sector::OK : Sector::BAD_CHECKSUM;
}
