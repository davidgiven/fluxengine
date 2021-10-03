#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "micropolis.h"
#include "bytes.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"

/* The sector has a preamble of MFM 0x00s and uses 0xFF as a sync pattern. */
static const FluxPattern SECTOR_SYNC_PATTERN(32, 0xaaaa5555);

/* Adds all bytes, with carry. */
uint8_t micropolisChecksum(const Bytes& bytes) {
	ByteReader br(bytes);
	uint16_t sum = 0;
	while (!br.eof()) {
		if (sum > 0xFF) {
			sum -= 0x100 - 1;
		}
		sum += br.read_8();
	}
	/* The last carry is ignored */
	return sum & 0xFF;
}

class MicropolisDecoder : public AbstractDecoder
{
public:
	MicropolisDecoder(const DecoderProto& config):
		AbstractDecoder(config),
		_config(config.micropolis())
	{}

	RecordType advanceToNextRecord()
	{
		_fmr->seekToIndexMark();
		const FluxMatcher* matcher = nullptr;
		_sector->clock = _fmr->seekToPattern(SECTOR_SYNC_PATTERN, matcher);
		if (matcher == &SECTOR_SYNC_PATTERN) {
			readRawBits(16);
			return SECTOR_RECORD;
		}
		return UNKNOWN_RECORD;
	}

	void decodeSectorRecord()
	{
		auto rawbits = readRawBits(MICROPOLIS_ENCODED_SECTOR_SIZE*16);
		auto bytes = decodeFmMfm(rawbits).slice(0, MICROPOLIS_ENCODED_SECTOR_SIZE);
		ByteReader br(bytes);

		br.read_8();  /* sync */
		_sector->logicalTrack = br.read_8();
		_sector->logicalSide = _sector->physicalHead;
		_sector->logicalSector = br.read_8();
		if (_sector->logicalSector > 15)
			return;
		if (_sector->logicalTrack > 77)
			return;

		br.read(10);  /* OS data or padding */
		auto data = br.read(256);
		uint8_t wantChecksum = br.read_8();
		uint8_t gotChecksum = micropolisChecksum(bytes.slice(1, 2+266));
		br.read(5);  /* 4 byte ECC and ECC-present flag */

		if (_config.sector_output_size() == 256)
			_sector->data = data;
		else if (_config.sector_output_size() == MICROPOLIS_ENCODED_SECTOR_SIZE)
			_sector->data = bytes;
		else
			Error() << "Sector output size may only be 256 or 275";
		_sector->status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}
private:
	const MicropolisDecoderProto& _config;
};

std::unique_ptr<AbstractDecoder> createMicropolisDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new MicropolisDecoder(config));
}

