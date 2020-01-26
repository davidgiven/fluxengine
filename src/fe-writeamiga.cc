#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "amiga/amiga.h"
#include "writer.h"
#include "fmt/format.h"
#include "image.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &amigaEncoderFlags };

int mainWriteAmiga(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=80:h=2:s=11:b=512");
    setWriterDefaultDest(":d=0:t=0-79:s=0-1");
    flags.parseFlags(argc, argv);

	AmigaEncoder encoder;
	writeDiskCommand(encoder);

    return 0;
}

