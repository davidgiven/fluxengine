#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "macintosh/macintosh.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &macintoshEncoderFlags };

int mainWriteMac(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=80:h=2:s=12:b=524");
	setWriterDefaultDest(":d=0:t=0-79:s=0-1");
    flags.parseFlags(argc, argv);

	MacintoshEncoder encoder;
	writeDiskCommand(encoder);

    return 0;
}


