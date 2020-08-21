#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "tids990.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"

FlagGroup tids990EncoderFlags;

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	3.83);

static DoubleFlag postIndexGapMs(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (milliseconds).",
	1.0);

static DoubleFlag sectorSpacingMs(
	{ "--sector-spacing" },
	"Time between successive sector headers (milliseconds).",
	16.2);

static DoubleFlag postHeaderSpacingMs(
	{ "--post-header-spacing" },
	"Time between a sector's header and data records (milliseconds).",
	0.69);

static StringFlag sectorSkew(
	{ "--sector-skew" },
	"Order in which to write sectors.",
	"05a3816b4927");

static int charToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

std::unique_ptr<Fluxmap> TiDs990Encoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	int logicalTrack;
	if (physicalSide != 0)
		return std::unique_ptr<Fluxmap>();
//	physicalTrack -= _bias;
//	switch (_format)
//	{
//		case 120:
//			if ((physicalTrack < 0) || (physicalTrack >= (BROTHER_TRACKS_PER_120KB_DISK*2))
//					|| (physicalTrack & 1))
//				return std::unique_ptr<Fluxmap>();
//			logicalTrack = physicalTrack/2;
//			break;
//
//		case 240:
//			if ((physicalTrack < 0) || (physicalTrack >= BROTHER_TRACKS_PER_240KB_DISK))
//				return std::unique_ptr<Fluxmap>();
//			logicalTrack = physicalTrack;
//			break;
//	}
//
//	int bitsPerRevolution = 200000.0 / clockRateUs;
//	const std::string& skew = sectorSkew.get();
//	std::vector<bool> bits(bitsPerRevolution);
//	unsigned cursor = 0;
//
//	for (int sectorCount=0; sectorCount<BROTHER_SECTORS_PER_TRACK; sectorCount++)
//	{
//		int sectorId = charToInt(skew.at(sectorCount));
//		double headerMs = postIndexGapMs + sectorCount*sectorSpacingMs;
//		unsigned headerCursor = headerMs*1e3 / clockRateUs;
//		double dataMs = headerMs + postHeaderSpacingMs;
//		unsigned dataCursor = dataMs*1e3 / clockRateUs;
//
//		const auto& sectorData = allSectors.get(logicalTrack, 0, sectorId);
//
//		fillBitmapTo(bits, cursor, headerCursor, { true, false });
//		write_sector_header(bits, cursor, logicalTrack, sectorId);
//		fillBitmapTo(bits, cursor, dataCursor, { true, false });
//		write_sector_data(bits, cursor, sectorData->data);
//	}
//
//	if (cursor >= bits.size())
//		Error() << "track data overrun";
//	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	// The pre-index gap is not normally reported.
	// std::cerr << "pre-index gap " << 200.0 - (double)cursor*clockRateUs/1e3 << std::endl;
	
	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
//	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}

