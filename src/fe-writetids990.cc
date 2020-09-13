#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "tids990/tids990.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &tids990EncoderFlags };

int mainWriteTiDs990(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=77:h=2:s=26:b=288");
	setWriterDefaultDest(":d=0:t=0-76:s=0-1");
    flags.parseFlags(argc, argv);

	TiDs990Encoder encoder;
	writeDiskCommand(encoder);

    return 0;
}

