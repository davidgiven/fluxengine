#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "victor9k.h"
#include "crc.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(32, VICTOR9K_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(32, VICTOR9K_DATA_RECORD);
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
}

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

        uint8_t decoded = decode_data_gcr(inputfifo);
        bitw.push(decoded, 4);
    }
    bitw.flush();

    return output;
}

class Victor9kDecoder : public AbstractDecoder
{
public:
	Victor9kDecoder(const DecoderProto& config):
		AbstractDecoder(config)
	{}

    RecordType advanceToNextRecord()
	{
		const FluxMatcher* matcher = nullptr;
		_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);
		if (matcher == &SECTOR_RECORD_PATTERN)
			return SECTOR_RECORD;
		if (matcher == &DATA_RECORD_PATTERN)
			return DATA_RECORD;
		return UNKNOWN_RECORD;
	}

    void decodeSectorRecord()
	{
		/* Skip the sync marker bits, a series of 1s. */
		while (readRawBits(1)[0]) { };

		//get next bits and recover above false bit from while exit 
		auto firstBytes = readRawBits(9 + (3*10));
		firstBytes.insert(firstBytes.begin(), false);
	
		/* Read header. */
		auto bytes = decode(firstBytes).slice(0, 4);
    	
		uint8_t headerID = bytes[0];
		uint8_t rawTrack = bytes[1];
		_sector->logicalSector = bytes[2];
		uint8_t gotChecksum = bytes[3];

		_sector->logicalTrack = rawTrack & 0x7f;
		_sector->logicalSide = rawTrack >> 7;
		uint8_t wantChecksum = bytes[1] + bytes[2];
		if ((_sector->logicalSector > 20) || (_sector->logicalTrack > 85) || (_sector->logicalSide > 1))
			return;
					
		if (wantChecksum == gotChecksum)
			_sector->status = Sector::DATA_MISSING; /* unintuitive but correct */
	}

    void decodeDataRecord()
	{
		/* Skip the sync marker bit. */
		std::vector<bool> startbits = readRawBits(22);

		/* Read data. */

		auto bytes = decode(readRawBits((VICTOR9K_SECTOR_LENGTH+5)*10))
			.slice(0, VICTOR9K_SECTOR_LENGTH+5);
		ByteReader br(bytes);

		/* Check that this is actually a data record. */
		
		if (br.read_8() != 8)
			return;

		_sector->data = br.read(VICTOR9K_SECTOR_LENGTH);
		uint16_t gotChecksum = sumBytes(_sector->data);
		uint16_t wantChecksum = br.read_le16();
		_sector->status = (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}
};

std::unique_ptr<AbstractDecoder> createVictor9kDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new Victor9kDecoder(config));
}


