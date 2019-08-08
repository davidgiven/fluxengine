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

int mainWriteBrother(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=78:h=1:s=12:b=256");
	setWriterDefaultDest(":d=0:t=0-77:s=0");
    flags.parseFlags(argc, argv);

	BrotherEncoder encoder;
	writeDiskCommand(encoder);

    return 0;
}

