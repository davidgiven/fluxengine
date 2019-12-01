#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "amiga.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"

FlagGroup amigaEncoderFlags;

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	2.00);

static DoubleFlag postIndexGapMs(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (milliseconds).",
	20.0);

static int charToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
	cursor += width;
	for (int i=0; i<width; i++)
	{
		unsigned pos = cursor - i - 1;
		if (pos < bits.size())
			bits[pos] = data & 1;
		data >>= 1;
	}
}

static void write_sector(std::vector<bool> bits, unsigned& cursor, const Sector* sector)
{
    write_bits(bits, cursor, AMIGA_SECTOR_RECORD, 6*8);

    Bytes header(4);
    ByteWriter bw(header);
    bw.write_8(0xff); /* Amiga 1.0 format byte */
    bw.write_8(sector->logicalTrack);
    bw.write_8(sector->logicalSector);
    bw.write_8(AMIGA_SECTORS_PER_TRACK - sector->logicalSector);
}

std::unique_ptr<Fluxmap> AmigaEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= AMIGA_TRACKS_PER_DISK))
		return std::unique_ptr<Fluxmap>();

	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

    fillBitmapTo(bits, cursor, postIndexGapMs * 1000 / clockRateUs, { true, false });

	for (int sectorId=0; sectorId<AMIGA_SECTORS_PER_TRACK; sectorId++)
	{
		const auto& sectorData = allSectors.get(physicalTrack, 0, sectorId);
		write_sector(bits, cursor, sectorData);
    }

	if (cursor > bits.size())
		Error() << "track data overrun";
	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}

