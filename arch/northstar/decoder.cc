/* Decoder for North Star 10-sector hard-sectored disks.
 *
 * Supports both single- and double-density.  For the sector format and
 * checksum algorithm, see pp. 33 of the North Star Double Density Controller
 * manual:
 *
 * http://bitsavers.org/pdf/northstar/boards/Northstar_MDS-A-D_1978.pdf
 *
 * North Star disks do not contain any track/head/sector information
 * encoded in the sector record.  For this reason, we have to be absolutely
 * sure that the hardSectorId is correct.
 */

#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "northstar.h"
#include "bytes.h"
#include "lib/decoders/decoders.pb.h"
#include "fmt/format.h"

/*
 * MFM sectors have 32 bytes of 00's followed by two sync characters,
 * specified in the North Star MDS manual as 0xFBFB.
 *
 * This is true for most disks; however, I found a few disks, including an
 * original North Star DOS/BASIC v2.2.1 DQ disk) that uses 0xFBnn, where
 * nn is an incrementing pattern.
 *
 * 00        00        00        F         B
 * 0000 0000 0000 0000 0000 0000 0101 0101 0100 0101
 * A    A    A    A    A    A    5    5    4    5
 */
static const FluxPattern MFM_PATTERN(64, 0xAAAAAAAAAAAA5545LL);

/* FM sectors have 16 bytes of 00's followed by 0xFB.
 * 00        FB
 * 0000 0000 1111 1111 1110 1111
 * A    A    F    F    E    F
 */
static const FluxPattern FM_PATTERN(64, 0xAAAAAAAAAAAAFFEFLL);

const FluxMatchers ANY_SECTOR_PATTERN(
	{
		&MFM_PATTERN,
		&FM_PATTERN,
	}
);

/* Checksum is initially 0.
 * For each data byte, XOR with the current checksum.
 * Rotate checksum left, carrying bit 7 to bit 0.
 */
uint8_t northstarChecksum(const Bytes& bytes) {
	ByteReader br(bytes);
	uint8_t checksum = 0;

	while (!br.eof()) {
		checksum ^= br.read_8();
		checksum = ((checksum << 1) | ((checksum >> 7)));
	}

	return checksum;
}

class NorthstarDecoder : public AbstractDecoder
{
public:
	NorthstarDecoder(const DecoderProto& config):
		AbstractDecoder(config),
		_config(config.northstar())
	{}

	/* Search for FM or MFM sector record */
	RecordType advanceToNextRecord() override
	{
		nanoseconds_t now = _fmr->tell().ns();

		/* For all but the first sector, seek to the next sector pulse.
		 * The first sector does not contain the sector pulse in the fluxmap.
		 */
		if (now != 0) {
			_fmr->seekToIndexMark();
			now = _fmr->tell().ns();
		}

		/* Discard a possible partial sector at the end of the track.
		 * This partial sector could be mistaken for a conflicted sector, if
		 * whatever data read happens to match the checksum of 0, which is
		 * rare, but has been observed on some disks.
		 */
		if (now > (_fmr->getDuration() - 21e6)) {
			_fmr->seekToIndexMark();
			return(UNKNOWN_RECORD);
		}

		int msSinceIndex = std::round(now / 1e6);

		const FluxMatcher* matcher = nullptr;

		/* Note that the seekToPattern ignores the sector pulses, so if
		 * a sector is not found for some reason, the seek will advance
		 * past one or more sector pulses.  For this reason, calculate
		 * _hardSectorId after the sector header is found.
		 */
		_sector->clock = _fmr->seekToPattern(ANY_SECTOR_PATTERN, matcher);

		int sectorFoundTimeRaw = std::round((_fmr->tell().ns()) / 1e6);
		int sectorFoundTime;

		/* Round time to the nearest 20ms */
		if ((sectorFoundTimeRaw % 20) < 10) {
			sectorFoundTime = (sectorFoundTimeRaw / 20) * 20;
		}
		else {
			sectorFoundTime = ((sectorFoundTimeRaw + 20) / 20) * 20;
		}

		/* Calculate the sector ID based on time since the index */
		_hardSectorId = (sectorFoundTime / 20) % 10;

	//	std::cout << fmt::format(
	//		"Sector ID {}: hole at {}ms, sector start at {}ms",
	//		_hardSectorId, msSinceIndex, sectorFoundTimeRaw) << std::endl;

		if (matcher == &MFM_PATTERN) {
			_sectorType = SECTOR_TYPE_MFM;
			return SECTOR_RECORD;
		}

		if (matcher == &FM_PATTERN) {
			_sectorType = SECTOR_TYPE_FM;
			return SECTOR_RECORD;
		}

		return UNKNOWN_RECORD;
	}

	void decodeSectorRecord() override
	{
		unsigned recordSize, payloadSize, headerSize;

		if (_sectorType == SECTOR_TYPE_MFM) {
			recordSize = NORTHSTAR_ENCODED_SECTOR_SIZE_DD;
			payloadSize = NORTHSTAR_PAYLOAD_SIZE_DD;
			headerSize = NORTHSTAR_HEADER_SIZE_DD;
		}
		else {
			recordSize = NORTHSTAR_ENCODED_SECTOR_SIZE_SD;
			payloadSize = NORTHSTAR_PAYLOAD_SIZE_SD;
			headerSize = NORTHSTAR_HEADER_SIZE_SD;
		}

		readRawBits(48);

		auto rawbits = readRawBits(recordSize * 16);
		auto bytes = decodeFmMfm(rawbits).slice(0, recordSize);
		ByteReader br(bytes);
		uint8_t sync_char;

		_sector->logicalSide = _sector->physicalHead;
		_sector->logicalSector = _hardSectorId;
		_sector->logicalTrack = _sector->physicalCylinder;

		sync_char = br.read_8();	/* Sync char: 0xFB */
		if (_sectorType == SECTOR_TYPE_MFM) {
			sync_char = br.read_8();/* MFM second Sync char, usually 0xFB */
		}

		_sector->data = br.read(payloadSize);

		uint8_t wantChecksum = br.read_8();
		uint8_t gotChecksum = northstarChecksum(bytes.slice(headerSize, payloadSize));

		_sector->status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}

	std::set<unsigned> requiredSectors(unsigned cylinder, unsigned head) const override
	{
		static std::set<unsigned> sectors = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		return sectors;
	}

private:
	const NorthstarDecoderProto& _config;
	uint8_t _sectorType = SECTOR_TYPE_MFM;
	uint8_t _hardSectorId;
};

std::unique_ptr<AbstractDecoder> createNorthstarDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new NorthstarDecoder(config));
}

