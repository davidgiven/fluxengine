#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "zilogmcz/zilogmcz.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadZilogMCZ(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-76:s=0");
	setReaderDefaultOutput("zilogmcz.img");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	ZilogMczDecoder decoder;
	readDiskCommand(decoder);
    return 0;
}

