#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "zilogmcz.h"
#include <fmt/format.h>

static FlagGroup flags { &readerFlags };

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "zilogmcz.img");

int mainReadZilogMCZ(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-76:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	ZilogMczDecoder decoder;
	readDiskCommand(decoder, outputFilename);
    return 0;
}

