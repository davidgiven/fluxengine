#include "globals.h"
#include "flags.h"
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

int main(int argc, const char* argv[])
{
	setWriterDefaultDest(":d=0:t=0-77:s=0");
    Flag::parseFlags(argc, argv);

	SectorSet allSectors;
	Geometry geometry = {78, 1, 12, 256};
	readSectorsFromFile(allSectors, geometry, inputFilename);

	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::cerr << bitsPerRevolution << " bits per 200ms revolution" << std::endl
	          << fmt::format("post-index gap: {:.3f}ms\n", (double)postIndexGapMs);

	const std::string& skew = sectorSkew;

	writeTracks(
		[&](int track, int side) -> std::unique_ptr<Fluxmap>
		{
			if ((track < 0) || (track > 77) || (side != 0))
				return std::unique_ptr<Fluxmap>();

			std::vector<bool> bits(bitsPerRevolution);
			unsigned cursor = 0;

			for (int sectorCount=0; sectorCount<geometry.sectors; sectorCount++)
			{
				int sectorId = charToInt(skew.at(sectorCount));
				double headerMs = postIndexGapMs + sectorCount*sectorSpacingMs;
				unsigned headerCursor = headerMs*1e3 / clockRateUs;
				double dataMs = headerMs + postHeaderSpacingMs;
				unsigned dataCursor = dataMs*1e3 / clockRateUs;

				auto& sectorData = allSectors.get(track, 0, sectorId);

				fillBitmapTo(bits, cursor, headerCursor, { true, false });
				writeBrotherSectorHeader(bits, cursor, track, sectorId);
				fillBitmapTo(bits, cursor, dataCursor, { true, false });
				writeBrotherSectorData(bits, cursor, sectorData->data);
			}

			if (cursor > bits.size())
				Error() << "track data overrun";
            fillBitmapTo(bits, cursor, bits.size(), { true, false });

			// The pre-index gap is not normally reported.
			// std::cerr << "pre-index gap " << 200.0 - (double)cursor*clockRateUs/1e3 << std::endl;
			
			std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
			fluxmap->appendBits(bits, clockRateUs*1e3);
			return fluxmap;
		}
	);

    return 0;
}

