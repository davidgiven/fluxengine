#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "tids990/tids990.h"
#include "fmt/format.h"

static FlagGroup flags { &readerFlags };

int mainReadTiDs990(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-76");
	setReaderDefaultOutput("tids990.img");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	TiDs990Decoder decoder;
	readDiskCommand(decoder);
    return 0;
}


