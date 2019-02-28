#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "victor.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include <fmt/format.h>
#include <fstream>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "victor.img");

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
    setReaderRevolutions(2);
    Flag::parseFlags(argc, argv);

	VictorDecoder decoder;
	readDiskCommand(decoder, outputFilename);

    return 0;
}

