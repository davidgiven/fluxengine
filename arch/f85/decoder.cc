#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "f85.h"
#include "crc.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(24, F85_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(24, F85_DATA_RECORD);
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

static Bytes decode(const std::vector<bool>& bits)
{
    Bytes output;
    ByteWriter bw(output);
    BitWriter bitw(bw);

    auto ii = bits.begin();
    while (ii != bits.end())
    {
        uint8_t inputfifo = 0;
        for (size_t i=0; i<5; i++)
        {
            if (ii == bits.end())
                break;
            inputfifo = (inputfifo<<1) | *ii++;
        }

        bitw.push(decode_data_gcr(inputfifo), 4);
    }
    bitw.flush();

    return output;
}

AbstractDecoder::RecordType DurangoF85Decoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);
	if (matcher == &SECTOR_RECORD_PATTERN)
		return RecordType::SECTOR_RECORD;
	if (matcher == &DATA_RECORD_PATTERN)
		return RecordType::DATA_RECORD;
	return RecordType::UNKNOWN_RECORD;
}

void DurangoF85Decoder::decodeSectorRecord()
{
    /* Skip sync bits and ID byte. */

    readRawBits(24);

    /* Read header. */

    const auto& bytes = decode(readRawBits(6*10));

    _sector->logicalSector = bytes[2];
    _sector->logicalSide = 0;
    _sector->logicalTrack = bytes[0];

    uint16_t wantChecksum = bytes.reader().seek(4).read_be16();
    uint16_t gotChecksum = crc16(CCITT_POLY, 0xef21, bytes.slice(0, 4));
    if (wantChecksum == gotChecksum)
        _sector->status = Sector::DATA_MISSING; /* unintuitive but correct */
}

void DurangoF85Decoder::decodeDataRecord()
{
    /* Skip sync bits ID byte. */

    readRawBits(24);

    const auto& bytes = decode(readRawBits((F85_SECTOR_LENGTH+3)*10))
        .slice(0, F85_SECTOR_LENGTH+3);
    ByteReader br(bytes);

    _sector->data = br.read(F85_SECTOR_LENGTH);
    uint16_t wantChecksum = br.read_be16();
    uint16_t gotChecksum = crc16(CCITT_POLY, 0xbf84, _sector->data);
    _sector->status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
