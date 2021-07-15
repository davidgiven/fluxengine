#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "record.h"
#include "northstar/northstar.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadNorthstar(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-34");
	setReaderDefaultOutput("northstar.nsi");
	setReaderHardSectorCount(10);
	setReaderFluxSourceSynced(true);
	flags.parseFlags(argc, argv);

	NorthstarDecoder decoder;
	readDiskCommand(decoder);
	return 0;
}

