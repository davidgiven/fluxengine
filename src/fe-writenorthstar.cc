#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "northstar/northstar.h"
#include "writer.h"

static FlagGroup flags { &writerFlags, &northstarEncoderFlags };

int mainWriteNorthstar(int argc, const char* argv[])
{
	setWriterDefaultDest(":t=0-34");
	setWriterHardSectorCount(10);
	flags.parseFlags(argc, argv);

	NorthstarEncoder encoder;
	writeDiskCommand(encoder);

	return 0;
}

