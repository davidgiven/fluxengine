#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "aeslanier/aeslanier.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadAESLanier(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
	setReaderDefaultOutput("aeslanier.img");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	AesLanierDecoder decoder;
	readDiskCommand(decoder);
    return 0;
}

