#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { };

static void syntax()
{
    std::cout << "Syntax: fluxengine convert image <srcspec> <destspec>\n";
    exit(0);
}

int mainConvertImage(int argc, const char* argv[])
{
    auto filenames = flags.parseFlagsWithFilenames(argc, argv);
    if (filenames.size() != 2)
        syntax();

	DataSpec ids(filenames[0]);
	ImageSpec iis(ids);
	SectorSet sectors = ImageReader::create(iis)->readImage();

	DataSpec ods(filenames[1]);
	ImageSpec ois(ods);
	auto writer = ImageWriter::create(sectors, ois);
	writer->adjustGeometry();
	writer->printMap();
	writer->writeImage();

    return 0;
}

