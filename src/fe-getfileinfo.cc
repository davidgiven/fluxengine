#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
#include "readerwriter.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({ &fileFlags });

static StringFlag directory({"-p", "--path"}, "path to work on", "");

int mainGetFileInfo(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("ls", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	auto filesystem = createFilesystemFromConfig();
	auto attributes = filesystem->getMetadata(Path(directory));

	for (const auto& e : attributes)
		fmt::print("{} = {}\n", e.first, e.second);

    return 0;
}
