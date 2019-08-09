#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "apple2/apple2.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &readerFlags };

int mainReadApple2(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
	setReaderDefaultOutput("apple2.adf");
    setReaderRevolutions(2);
    flags.parseFlags(argc, argv);

	Apple2Decoder decoder;
	readDiskCommand(decoder);

    return 0;
}
