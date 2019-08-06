#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "brother/brother.h"
#include "writer.h"
#include "fmt/format.h"
#include "image.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &brotherEncoderFlags };

static StringFlag inputFilename(
    { "--input", "-i" },
    "The input image file to read from.",
    "brother.img");

int mainWriteBrother(int argc, const char* argv[])
{
	setWriterDefaultDest(":d=0:t=0-77:s=0");
    flags.parseFlags(argc, argv);

	BrotherEncoder encoder;
	Geometry geometry = {78, 1, 12, 256};
	writeDiskCommand(encoder, geometry, inputFilename);

    return 0;
}

