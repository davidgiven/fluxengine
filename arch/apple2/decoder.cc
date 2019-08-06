#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "apple2.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(24, APPLE2_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(24, APPLE2_DATA_RECORD);
const FluxMatchers ANY_RECORD_PATTERN({ &SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN });

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

/* This is extremely inspired by the MESS implementation, written by Nathan Woods
 * and R. Belmont: https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib/formats/ap2_dsk.cpp
 */
static Bytes decode_crazy_data(const uint8_t* inp, Sector::Status& status)
{
    Bytes output(APPLE2_SECTOR_LENGTH);

    uint8_t checksum = 0;
    for (unsigned i = 0; i < APPLE2_ENCODED_SECTOR_LENGTH; i++)
    {
        checksum ^= decode_data_gcr(*inp++);

        if (i >= 86)
        {
            /* 6 bit */
            output[i - 86] |= (checksum << 2);
        }
        else
        {
            /* 3 * 2 bit */
            output[i + 0] = ((checksum >> 1) & 0x01) | ((checksum << 1) & 0x02);
            output[i + 86] = ((checksum >> 3) & 0x01) | ((checksum >> 1) & 0x02);
            if ((i + 172) < APPLE2_SECTOR_LENGTH)
                output[i + 172] = ((checksum >> 5) & 0x01) | ((checksum >> 3) & 0x02);
        }
    }

    checksum &= 0x3f;
    uint8_t wantedchecksum = decode_data_gcr(*inp);
    status = (checksum == wantedchecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    return output;
}

uint8_t combine(uint16_t word)
{
    return word & (word >> 7);
}

AbstractDecoder::RecordType Apple2Decoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);
	if (matcher == &SECTOR_RECORD_PATTERN)
		return RecordType::SECTOR_RECORD;
	if (matcher == &DATA_RECORD_PATTERN)
		return RecordType::DATA_RECORD;
	return RecordType::UNKNOWN_RECORD;
}

void Apple2Decoder::decodeSectorRecord()
{
    /* Skip ID (as we know it's a APPLE2_SECTOR_RECORD). */
    readRawBits(24);

    /* Read header. */

    auto header = toBytes(readRawBits(8*8)).slice(0, 8);
    ByteReader br(header);

    uint8_t volume = combine(br.read_be16());
    _sector->logicalTrack = combine(br.read_be16());
    _sector->logicalSector = combine(br.read_be16());
    uint8_t checksum = combine(br.read_be16());
    if (checksum == (volume ^ _sector->logicalTrack ^ _sector->logicalSector))
        _sector->status = Sector::DATA_MISSING; /* unintuitive but correct */
}

void Apple2Decoder::decodeDataRecord()
{
    /* Check ID. */

    Bytes bytes = toBytes(readRawBits(3*8)).slice(0, 3);
    if (bytes.reader().read_be24() != APPLE2_DATA_RECORD)
        return;

    /* Read and decode data. */

    unsigned recordLength = APPLE2_ENCODED_SECTOR_LENGTH + 2;
    bytes = toBytes(readRawBits(recordLength*8)).slice(0, recordLength);

    _sector->status = Sector::BAD_CHECKSUM;
    _sector->data = decode_crazy_data(&bytes[0], _sector->status);
}
