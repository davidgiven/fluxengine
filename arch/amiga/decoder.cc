#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "amiga.h"
#include "bytes.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/data.pb.h"
#include <string.h>
#include <algorithm>

/* 
 * Amiga disks use MFM but it's not quite the same as IBM MFM. They only use
 * a single type of record with a different marker byte.
 * 
 * See the big comment in the IBM MFM decoder for the gruesome details of how
 * MFM works.
 */
         
static const FluxPattern SECTOR_PATTERN(48, AMIGA_SECTOR_RECORD);

class AmigaDecoder : public AbstractDecoder
{
public:
	AmigaDecoder(const DecoderProto& config):
		AbstractDecoder(config),
		_config(config.amiga())
	{}

    RecordType advanceToNextRecord()
	{
		_sector->clock = _fmr->seekToPattern(SECTOR_PATTERN);
		if (_fmr->eof() || !_sector->clock)
			return UNKNOWN_RECORD;
		return SECTOR_RECORD;
	}

    void decodeSectorRecord()
	{
		const auto& rawbits = readRawBits(AMIGA_RECORD_SIZE*16);
		if (rawbits.size() < (AMIGA_RECORD_SIZE*16))
			return;
		const auto& rawbytes = toBytes(rawbits).slice(0, AMIGA_RECORD_SIZE*2);
		const auto& bytes = decodeFmMfm(rawbits).slice(0, AMIGA_RECORD_SIZE);

		const uint8_t* ptr = bytes.begin() + 3;

		Bytes header = amigaDeinterleave(ptr, 4);
		Bytes recoveryinfo = amigaDeinterleave(ptr, 16);

		_sector->logicalTrack = header[1] >> 1;
		_sector->logicalSide = header[1] & 1;
		_sector->logicalSector = header[2];

		uint32_t wantedheaderchecksum = amigaDeinterleave(ptr, 4).reader().read_be32();
		uint32_t gotheaderchecksum = amigaChecksum(rawbytes.slice(6, 40));
		if (gotheaderchecksum != wantedheaderchecksum)
			return;

		uint32_t wanteddatachecksum = amigaDeinterleave(ptr, 4).reader().read_be32();
		uint32_t gotdatachecksum = amigaChecksum(rawbytes.slice(62, 1024));

		Bytes data;
		data.writer().append(amigaDeinterleave(ptr, 512)).append(recoveryinfo);
		_sector->data = data;
		_sector->status = (gotdatachecksum == wanteddatachecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}

	std::set<unsigned> requiredSectors(Track& track) const
	{
		static std::set<unsigned> sectors = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		return sectors;
	}

private:
	const AmigaDecoderProto& _config;
};

std::unique_ptr<AbstractDecoder> createAmigaDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new AmigaDecoder(config));
}

