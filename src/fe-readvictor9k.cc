#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "victor9k/victor9k.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include <fmt/format.h>
#include <fstream>

static FlagGroup flags { &readerFlags };

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "victor9k.img");

int mainReadVictor9K(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	Victor9kDecoder decoder;
	readDiskCommand(decoder, outputFilename);

    return 0;
}

