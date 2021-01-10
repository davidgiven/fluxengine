#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "micropolis/micropolis.h"
#include "writer.h"

static FlagGroup flags { &writerFlags, &micropolisEncoderFlags };

int mainWriteMicropolis(int argc, const char* argv[])
{
	setWriterDefaultInput(":c=77:h=2:s=16:b=256");
	setWriterDefaultDest(":t=0-76");
	setWriterHardSectorCount(16);
	flags.parseFlags(argc, argv);

	MicropolisEncoder encoder;
	writeDiskCommand(encoder);

	return 0;
}

