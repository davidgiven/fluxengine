#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "mx/mx.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include <fmt/format.h>

static FlagGroup flags { &readerFlags };

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "mx.img");

int mainReadMx(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
    flags.parseFlags(argc, argv);

	MxDecoder decoder;
	readDiskCommand(decoder, outputFilename);
    return 0;
}

