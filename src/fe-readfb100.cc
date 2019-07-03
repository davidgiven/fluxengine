#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "fb100/fb100.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "fb100.img");

int mainReadFB100(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79x2:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	Fb100Decoder decoder;
	readDiskCommand(decoder, outputFilename);
    return 0;
}

