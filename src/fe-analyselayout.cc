#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "dep/agg/include/agg2d.h"
#include "dep/stb/stb_image_write.h"
#include <fstream>

static FlagGroup flags = {
};

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":d=0:t=0:s=0");

static DataSpecFlag writeImg(
	{ "--write-img" },
	"Draw a graph of the disk layout",
	":w=640:h=480");

int mainAnalyseLayout(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
	return 0;
}

