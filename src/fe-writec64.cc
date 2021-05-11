#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "c64/c64.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &Commodore64EncoderFlags };

int mainWriteC64(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=40:h=1:s=21:b=256");
	setWriterDefaultDest(":d=0:t=0-39:s=0");
    flags.parseFlags(argc, argv);

	Commodore64Encoder encoder;
	writeDiskCommand(encoder);

    return 0;
}


