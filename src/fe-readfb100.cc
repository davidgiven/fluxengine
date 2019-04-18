#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "fb100.h"
#include <fmt/format.h>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "fb100.img");

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79x2:s=0");
    setReaderRevolutions(2);
    setDecoderManualClockRate(4.0);
    Flag::parseFlags(argc, argv);

	Fb100Decoder decoder;
	readDiskCommand(decoder, outputFilename);
    return 0;
}

