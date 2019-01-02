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

static IntFlag clockRate(
	{ "--clock-rate" },
	"Encoded data clock rate (nanoseconds).",
	3850);

static IntFlag postIndexGap(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (nanoseconds).",
	100000);

static IntFlag sectorSpacing(
	{ "--sector-spacing" },
	"Time between successive sector heads (nanoseconds).",
	100000);

static StringFlag sectorSkew(
	{ "--sector-skew" },
	"Order in which to write sectors.",
	"0123456789ab");

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

	SectorSet allSectors;
	Geometry geometry = {78, 1, 12, 256};
	readSectorsFromFile(allSectors, geometry, inputFilename);

	int lastSectorAt = postIndexGap + geometry.sectors*sectorSpacing;
	if (lastSectorAt > 200000000)
		Error() << "with that spacing, some sectors won't fit in a track";

	int bitsPerRevolution = 200000000 / clockRate;
	std::cerr << bitsPerRevolution << " bits per 200ms revolution" << std::endl
	          << "post-index gap: " << postIndexGap << "ns" << std::endl
			  << "pre-index gap: " << (200000000 - lastSectorAt) << "ns" << std::endl;

	for (int track=0; track<geometry.tracks; track++)
	{
		std::vector<bool> bits(bitsPerRevolution);

		for (int sectorCount=0; sectorCount<geometry.sectors; sectorCount++)
		{
			int cursor = postIndexGap + sectorCount*sectorSpacing;
		}

	}

	std::cerr << "Not implemented yet." << std::endl;
    return 0;
}

