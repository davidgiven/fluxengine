#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

static StringFlag directory({"-p", "--path"}, "disk path to work on", "");
static StringFlag output({"-l", "--local"}, "local filename to write to", "");

int mainGetFile(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("getfile", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        Path inputFilename(directory);
        if (inputFilename.size() == 0)
            error("you must supply a filename to read");

        std::string outputFilename = output;
        if (outputFilename.empty())
            outputFilename = inputFilename.back();

        auto filesystem = Filesystem::createFilesystemFromConfig();
        auto data = filesystem->getFile(inputFilename);
        data.writeToFile(outputFilename);
    }
    catch (const FilesystemException& e)
    {
        error("{}", e.message);
    }

    return 0;
}
