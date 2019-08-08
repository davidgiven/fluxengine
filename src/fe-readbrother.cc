#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "brother/brother.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &readerFlags };

int mainReadBrother(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-81:s=0");
	setReaderDefaultOutput("brother.img");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	BrotherDecoder decoder;
	readDiskCommand(decoder);

    return 0;
}

