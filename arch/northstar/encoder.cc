#include "globals.h"
#include "northstar.h"
#include "sectorset.h"

#define GAP_FILL_SIZE_SD 30
#define PRE_HEADER_GAP_FILL_SIZE_SD 9
#define GAP_FILL_SIZE_DD 62
#define PRE_HEADER_GAP_FILL_SIZE_DD 16

#define GAP1_FILL_BYTE	(0x4F)
#define GAP2_FILL_BYTE	(0x4F)

#define TOTAL_SECTOR_BYTES ()

static void write_sector(std::vector<bool>& bits, unsigned& cursor, const Sector* sector)
{
	int preambleSize = 0;
	int encodedSectorSize = 0;
	int gapFillSize = 0;
	int preHeaderGapFillSize = 0;

	bool doubleDensity;

	switch (sector->data.size()) {
	case NORTHSTAR_PAYLOAD_SIZE_SD:
		preambleSize = NORTHSTAR_PREAMBLE_SIZE_SD;
		encodedSectorSize = PRE_HEADER_GAP_FILL_SIZE_SD + NORTHSTAR_ENCODED_SECTOR_SIZE_SD + GAP_FILL_SIZE_SD;
		gapFillSize = GAP_FILL_SIZE_SD;
		preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_SD;
		doubleDensity = false;
		break;
	case NORTHSTAR_PAYLOAD_SIZE_DD:
		preambleSize = NORTHSTAR_PREAMBLE_SIZE_DD;
		encodedSectorSize = PRE_HEADER_GAP_FILL_SIZE_DD + NORTHSTAR_ENCODED_SECTOR_SIZE_DD + GAP_FILL_SIZE_DD;
		gapFillSize = GAP_FILL_SIZE_DD;
		preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_DD;
		doubleDensity = true;
		break;
	default:
		Error() << "unsupported sector size --- you must pick 256 or 512";
		break;
	}

	int fullSectorSize = preambleSize + encodedSectorSize;
	auto fullSector = std::make_shared<std::vector<uint8_t>>();
	fullSector->reserve(fullSectorSize);

	/* sector gap after index pulse */
	for (int i = 0; i < preHeaderGapFillSize; i++)
		fullSector->push_back(GAP1_FILL_BYTE);

	/* sector preamble */
	for (int i = 0; i < preambleSize; i++)
		fullSector->push_back(0);

	Bytes sectorData;
	if (sector->data.size() == encodedSectorSize)
		sectorData = sector->data;
	else {
		ByteWriter writer(sectorData);
		writer.write_8(0xFB);     /* sync character */
		if (doubleDensity == true) {
			writer.write_8(0xFB); /* Double-density has two sync characters */
		}
		writer += sector->data;
		if (doubleDensity == true) {
			writer.write_8(northstarChecksum(sectorData.slice(2)));
		} else {
			writer.write_8(northstarChecksum(sectorData.slice(1)));
		}
	}
	for (uint8_t b : sectorData)
		fullSector->push_back(b);

	if (sector->logicalSector != 9) {
		/* sector postamble */
		for (int i = 0; i < gapFillSize; i++)
			fullSector->push_back(GAP2_FILL_BYTE);

		if (fullSector->size() != fullSectorSize)
			Error() << "sector mismatched length (" << sector->data.size() << ") expected: " << fullSector->size() << " got " << fullSectorSize;
	} else {
		/* sector postamble */
		for (int i = 0; i < gapFillSize; i++)
			fullSector->push_back(GAP2_FILL_BYTE);
	}

	bool lastBit = false;

	if (doubleDensity == true) {
		encodeMfm(bits, cursor, fullSector, lastBit);
	}
	else {
		encodeFm(bits, cursor, fullSector);
	}
}

std::unique_ptr<Fluxmap> NorthstarEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	int bitsPerRevolution = 100000;
	double clockRateUs = 4.00;

	if ((physicalTrack < 0) || (physicalTrack >= 35))
		return std::unique_ptr<Fluxmap>();

	const auto& sector = allSectors.get(physicalTrack, physicalSide, 0);

	if (sector->data.size() == NORTHSTAR_PAYLOAD_SIZE_SD) {
		bitsPerRevolution /= 2;		// FM
	} else {
		clockRateUs /= 2.00;
	}

	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

	for (int sectorId = 0; sectorId < 10; sectorId++)
	{
		const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
		write_sector(bits, cursor, sectorData);
	}

	if (cursor > bits.size())
		Error() << "track data overrun";

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs * 1e3);
	return fluxmap;
}
