#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "c64.h"
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
    "c64.img");

int mainReadC64(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79x2:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	Commodore64Decoder decoder;
	readDiskCommand(decoder, outputFilename);

    return 0;
}

