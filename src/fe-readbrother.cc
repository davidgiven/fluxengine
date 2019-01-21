#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "brother.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include <fmt/format.h>
#include <fstream>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "brother.img");

#define SECTOR_COUNT 12
#define TRACK_COUNT 78

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-81:s=0");
    Flag::parseFlags(argc, argv);

	BrotherBitmapDecoder bitmapDecoder;
	BrotherRecordParser recordParser;
	readDiskCommand(bitmapDecoder, recordParser, outputFilename);

    return 0;
}

