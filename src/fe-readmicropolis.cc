#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "micropolis/micropolis.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadMicropolis(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-76");
	setReaderDefaultOutput("micropolis.img");
	setReaderHardSectorCount(16);
	flags.parseFlags(argc, argv);

	MicropolisDecoder decoder;
	readDiskCommand(decoder);
	return 0;
}

