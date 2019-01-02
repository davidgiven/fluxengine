#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "sector.h"
#include "sectorset.h"
#include "decoders.h"
#include "brother.h"
#include "image.h"
#include <fmt/format.h>
#include <fstream>

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
	"Time between successive sector heads (milliseconds).",
	16.684);

static StringFlag sectorSkew(
	{ "--sector-skew" },
	"Order in which to write sectors.",
	"05a3816b4927");

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

	SectorSet allSectors;
	Geometry geometry = {78, 1, 12, 256};
	readSectorsFromFile(allSectors, geometry, inputFilename);

	double lastSectorAtMs = postIndexGapMs + geometry.sectors*sectorSpacingMs;
	if (lastSectorAtMs > 200.0)
		Error() << "with that spacing, some sectors won't fit in a track "
				<< fmt::format("({:.3f}ms out of 200.0ms)", lastSectorAtMs);

	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::cerr << bitsPerRevolution << " bits per 200ms revolution" << std::endl
	          << fmt::format("post-index gap: {:.3f}ms\n", postIndexGapMs)
			  << fmt::format("pre-index gap: {:.3f}ms\n", 200.0 - lastSectorAtMs);

	const std::string& skew = sectorSkew;
	for (int track=0; track<geometry.tracks; track++)
	{
		std::vector<bool> bits(bitsPerRevolution);

		for (int sectorCount=0; sectorCount<geometry.sectors; sectorCount++)
		{
			int sectorId = skew.at(sectorCount) - '0';
			double cursorMs = postIndexGapMs + sectorCount*sectorSpacingMs;
			std::cerr << "logical sector " << sectorId << " at pos " << cursorMs << std::endl;
		}

	}

	std::cerr << "Not implemented yet." << std::endl;
    return 0;
}

