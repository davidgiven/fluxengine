#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "ibm/ibm.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadQd(int argc, const char* argv[])
{
	setReaderDefaultSource(":qd=1");
	setReaderDefaultOutput("qd.img");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	IbmDecoder decoder(0);
	readDiskCommand(decoder);
    return 0;
}

