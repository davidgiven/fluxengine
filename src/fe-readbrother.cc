#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "brother/brother.h"
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
    "brother.img");

int mainReadBrother(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-81:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	BrotherDecoder decoder;
	readDiskCommand(decoder, outputFilename);

    return 0;
}

