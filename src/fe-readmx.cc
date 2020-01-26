#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "mx/mx.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"

static FlagGroup flags { &readerFlags };

int mainReadMx(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
	setReaderDefaultOutput("mx.img");
    flags.parseFlags(argc, argv);

	MxDecoder decoder;
	readDiskCommand(decoder);
    return 0;
}

