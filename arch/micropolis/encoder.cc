#include "globals.h"
#include "micropolis.h"
#include "sectorset.h"

FlagGroup micropolisEncoderFlags;

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	2.00);

static void write_sector(std::vector<bool>& bits, unsigned& cursor, const Sector* sector)
{
	if ((sector->data.size() != 256) && (sector->data.size() != MICROPOLIS_ENCODED_SECTOR_SIZE))
		Error() << "unsupported sector size --- you must pick 256 or 275";

	int fullSectorSize = 40 + MICROPOLIS_ENCODED_SECTOR_SIZE + 40 + 35;
	auto fullSector = std::make_shared<std::vector<uint8_t>>();
	fullSector->reserve(fullSectorSize);
	/* sector preamble */
	for (int i=0; i<40; i++)
		fullSector->push_back(0);
	Bytes sectorData;
	if (sector->data.size() == MICROPOLIS_ENCODED_SECTOR_SIZE)
		sectorData = sector->data;
	else
	{
		ByteWriter writer(sectorData);
		writer.write_8(0xff); /* Sync */
		writer.write_8(sector->logicalTrack);
		writer.write_8(sector->logicalSector);
		for (int i=0; i<10; i++)
			writer.write_8(0); /* Padding */
		writer += sector->data;
		writer.write_8(micropolisChecksum(sectorData.slice(1)));
		for (int i=0; i<5; i++)
			writer.write_8(0); /* 4 byte ECC and ECC not present flag */
	}
	for (uint8_t b : sectorData)
		fullSector->push_back(b);
	/* sector postamble */
	for (int i=0; i<40; i++)
		fullSector->push_back(0);
	/* filler */
	for (int i=0; i<35; i++)
		fullSector->push_back(0);

	if (fullSector->size() != fullSectorSize)
		Error() << "sector mismatched length";
	bool lastBit = false;
	encodeMfm(bits, cursor, fullSector, lastBit);
	/* filler */
	for (int i=0; i<5; i++)
	{
		bits[cursor++] = 1;
		bits[cursor++] = 0;
	}
}

std::unique_ptr<Fluxmap> MicropolisEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= 77))
		return std::unique_ptr<Fluxmap>();

	int bitsPerRevolution = 100000;
	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

	for (int sectorId=0; sectorId<16; sectorId++)
	{
		const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
		write_sector(bits, cursor, sectorData);
	}

	if (cursor != bits.size())
		Error() << "track data mismatched length";

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}
