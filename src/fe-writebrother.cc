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

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

	SectorSet allSectors;
	Geometry geometry = {78, 1, 12, 256};
	readSectorsFromFile(allSectors, geometry, inputFilename);

	std::cerr << "Not implemented yet." << std::endl;
    return 0;
}

