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

static IntFlag sectorIdBase(
	{ "--sector-id-base" },
	"Sector ID of the first sector.",
	1);

static BoolFlag ignoreSideByte(
	{ "--ignore-side-byte" },
	"Ignore the side byte in the sector ID, and use the physical side instead.",
	false);

int mainReadIBM(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
	setReaderDefaultOutput("ibm.img");
    flags.parseFlags(argc, argv);

	IbmDecoder decoder(sectorIdBase, ignoreSideByte);
	readDiskCommand(decoder);
    return 0;
}

