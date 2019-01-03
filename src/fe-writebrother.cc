#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "sector.h"
#include "sectorset.h"
#include "decoders.h"
#include "brother.h"
#include "image.h"
#include "writer.h"
#include <fmt/format.h>
#include <fstream>
#include <ctype.h>

static StringFlag inputFilename(
    { "--input", "-i" },
    "The input image file to read from.",
    "brother.img");

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	3.850);

static IntFlag postIndexGapMs(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (milliseconds).",
	2.0);

static DoubleFlag sectorSpacingMs(
	{ "--sector-spacing" },
	"Time between successive sector headers (milliseconds).",
	16.684);

static DoubleFlag postHeaderSpacingMs(
	{ "--post-header-spacing" },
	"Time between a sector's header and data records (milliseconds).",
	0.694);


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

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

	SectorSet allSectors;
	Geometry geometry = {78, 1, 12, 256};
	readSectorsFromFile(allSectors, geometry, inputFilename);

	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::cerr << bitsPerRevolution << " bits per 200ms revolution" << std::endl
	          << fmt::format("post-index gap: {:.3f}ms\n", (double)postIndexGapMs);

	const std::string& skew = sectorSkew;
	for (int track=0; track<geometry.tracks; track++)
	{
		std::vector<bool> bits(bitsPerRevolution);
		unsigned cursor = 0;

		std::cerr << "logical track " << track << std::endl;

		for (int sectorCount=0; sectorCount<geometry.sectors; sectorCount++)
		{
			int sectorId = charToInt(skew.at(sectorCount));
			double headerMs = postIndexGapMs + sectorCount*sectorSpacingMs;
			unsigned headerCursor = headerMs*1e3 / clockRateUs;
			double dataMs = headerMs + postHeaderSpacingMs;
			unsigned dataCursor = dataMs*1e3 / clockRateUs;

			fillBitmapTo(bits, cursor, headerCursor, { true, false });
			writeBrotherSectorHeader(bits, cursor, track, sectorId);
			fillBitmapTo(bits, cursor, dataCursor, { true, false });

			if (cursor > bits.size())
				Error() << "track data overrun";
		}

		Fluxmap fluxmap;
		fluxmap.appendBits(bits, clockRateUs*1e3);
	}

	std::cerr << "Not implemented yet." << std::endl;
    return 0;
}

