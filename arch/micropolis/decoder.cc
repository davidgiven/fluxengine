#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "micropolis.h"
#include "bytes.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"

/* The sector has a preamble of MFM 0x00s and uses 0xFF as a sync pattern.
 *
 * 00        00        00        F         F
 * 0000 0000 0000 0000 0000 0000 0101 0101 0101 0101
 * A    A    A    A    A    A    5    5    5    5
 */
static const FluxPattern SECTOR_SYNC_PATTERN(64, 0xAAAAAAAAAAAA5555LL);

/* Pattern to skip past current SYNC. */
static const FluxPattern SECTOR_ADVANCE_PATTERN(64, 0xAAAAAAAAAAAAAAAALL);

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

	nanoseconds_t advanceToNextRecord() override
	{
		nanoseconds_t now = tell().ns();

		/* For all but the first sector, seek to the next sector pulse.
		 * The first sector does not contain the sector pulse in the fluxmap.
		 */
		if (now != 0) {
			seekToIndexMark();
			now = tell().ns();
		}

		/* Discard a possible partial sector at the end of the track.
		 * This partial sector could be mistaken for a conflicted sector, if
		 * whatever data read happens to match the checksum of 0, which is
		 * rare, but has been observed on some disks.
		 */
		if (now > (getFluxmapDuration() - 12.5e6)) {
			seekToIndexMark();
			return 0;
		}

		nanoseconds_t clock = seekToPattern(SECTOR_SYNC_PATTERN);

		auto syncDelta = tell().ns() - now;
		/* Due to the weak nature of the Micropolis SYNC patern,
		 * it's possible to detect a false SYNC during the gap
		 * between the sector pulse and the write gate.  If the SYNC
		 * is detected less than 100uS after the sector pulse, search
		 * for another valid SYNC.
		 *
		 * Reference: Vector Micropolis Disk Controller Board Technical
		 * Information Manual, pp. 1-16.
		 */
		if ((syncDelta > 0) && (syncDelta < 100e3)) {
			seekToPattern(SECTOR_ADVANCE_PATTERN);
			clock = seekToPattern(SECTOR_SYNC_PATTERN);
		}

		return clock;
	}

	void decodeSectorRecord()
	{
		readRawBits(48);
		auto rawbits = readRawBits(MICROPOLIS_ENCODED_SECTOR_SIZE*16);
		auto bytes = decodeFmMfm(rawbits).slice(0, MICROPOLIS_ENCODED_SECTOR_SIZE);
		ByteReader br(bytes);

		int syncByte = br.read_8();  /* sync */
		if (syncByte != 0xFF)
			return;

		_sector->logicalTrack = br.read_8();
		_sector->logicalSide = _sector->physicalHead;
		_sector->logicalSector = br.read_8();
		if (_sector->logicalSector > 15)
			return;
		if (_sector->logicalTrack > 76)
			return;
		if (_sector->logicalTrack != _sector->physicalCylinder)
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

	std::set<unsigned> requiredSectors(unsigned cylinder, unsigned head) const override
	{
		static std::set<unsigned> sectors = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
		return sectors;
	}

private:
	const MicropolisDecoderProto& _config;
};

std::unique_ptr<AbstractDecoder> createMicropolisDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new MicropolisDecoder(config));
}

