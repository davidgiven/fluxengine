#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "apple2.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include <fmt/format.h>
#include <fstream>

FlagGroup flags;

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "apple2.img");

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	Apple2Decoder decoder;
	readDiskCommand(decoder, outputFilename);

    return 0;
}
