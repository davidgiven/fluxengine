#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "ibm.h"
#include <fmt/format.h>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "ibm.img");

static IntFlag sectorIdBase(
	{ "--sector-id-base" },
	"Sector ID of the first sector.",
	1);

static BoolFlag readFm(
	{ "--read-fm" },
	"Read FM disks rather than MFM disks.",
	false);

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
    Flag::parseFlags(argc, argv);

	if (readFm)
	{
		IbmFmDecoder decoder(sectorIdBase);
		readDiskCommand(decoder, outputFilename);
	}
	else
	{
		IbmMfmDecoder decoder(sectorIdBase);
		readDiskCommand(decoder, outputFilename);
	}
    return 0;
}

