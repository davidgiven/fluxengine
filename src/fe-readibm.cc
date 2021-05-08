#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "dataspec.h"
#include "ibm/ibm.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

IntFlag sectorIdBase(
	{ "--ibm-sector-id-base" },
	"Sector ID of the first sector.",
	1);

BoolFlag ignoreSideByte(
	{ "--ibm-ignore-side-byte" },
	"Ignore the side byte in the sector ID, and use the physical side instead.",
	false);

RangeFlag requiredSectors(
	{ "--ibm-required-sectors" },
	"A comma seperated list or range of sectors which must be on each track.",
	"");

static ActionFlag preset360(
	{ "--ibm-preset-360" },
	"Preset parameters for reading a 5.25\" 360kB disk in a 48tpi drive.",
	[] {
		setReaderDefaultSource(":t=0-39");
		requiredSectors.set("0,1,2,3,4,5,6,7,8");
	});

static ActionFlag preset360hd(
	{ "--ibm-preset-360-96tpi" },
	"Preset parameters for reading a 5.25\" 360kB disk in a 96tpi drive.",
	[] {
		setReaderDefaultSource(":t=0-79x2");
		requiredSectors.set("0,1,2,3,4,5,6,7,8");
	});

static ActionFlag preset1200(
	{ "--ibm-preset-1200" },
	"Preset parameters for reading a 5.25\" 1.2MB disk.",
	[] {
		requiredSectors.set("0,1,2,3,4,5,6,7,8,9,10,11,12,13,14");
	});

int mainReadIBM(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
	setReaderDefaultOutput("ibm.img");
    flags.parseFlags(argc, argv);

	IbmDecoder decoder(sectorIdBase, ignoreSideByte, requiredSectors);
	readDiskCommand(decoder);
    return 0;
}

