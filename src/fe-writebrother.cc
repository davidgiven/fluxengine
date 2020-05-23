#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "brother/brother.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags, &brotherEncoderFlags };

static int brotherFormat = 240;
static ActionFlag preset120(
	{ "--brother-preset-120" },
	"Write the Brother 120kB format instead of the 240kB one.",
	[] {
		setWriterDefaultInput(":c=39:h=1:s=12:b=256");
		brotherFormat = 120;
	});

static IntFlag bias(
	{ "--brother-track-bias" },
	"Shift the entire format this many tracks on the disk.",
	0);

int mainWriteBrother(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=78:h=1:s=12:b=256");
	setWriterDefaultDest(":d=0:t=0-77:s=0");
    flags.parseFlags(argc, argv);

	BrotherEncoder encoder(brotherFormat, bias);
	writeDiskCommand(encoder);

    return 0;
}

