#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "macintosh.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"
#include "fmt/format.h"
#include <ctype.h>

FlagGroup macintoshEncoderFlags;

static DoubleFlag postIndexGapUs(
	{ "--post-index-gap-us" },
	"Post-index gap before first sector header (microseconds).",
	500);

static bool lastBit;

static double clockRateUsForTrack(unsigned track)
{
	if (track < 16)
		return 2.65;
	if (track < 32)
		return 2.90;
	if (track < 48)
		return 3.20;
	return 4.00;
}

static unsigned sectorsForTrack(unsigned track)
{
	if (track < 16)
		return 12;
	if (track < 32)
		return 11;
	if (track < 48)
		return 10;
	return 9;
}

std::unique_ptr<Fluxmap> MacintoshEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= MAC_TRACKS_PER_DISK))
		return std::unique_ptr<Fluxmap>();

	double clockRateUs = clockRateUsForTrack(physicalTrack);
	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

    fillBitmapTo(bits, cursor, postIndexGapUs / clockRateUs, { true, false });
	lastBit = false;

	unsigned numSectors = sectorsForTrack(physicalTrack);
	for (int sectorId=0; sectorId<numSectors; sectorId++)
	{
		const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
		//write_sector(bits, cursor, sectorData);
    }

	if (cursor >= bits.size())
		Error() << "track data overrun";
	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}

