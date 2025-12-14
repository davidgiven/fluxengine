#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/algorithms/readerwriter.h"
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

static StringFlag path({"-p", "--path"}, "path to work on", "");
static StringFlag input({"-l", "--local"}, "local filename to read from", "");

int mainPutFile(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("putfile", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        std::string inputFilename = input;
        if (inputFilename.empty())
            error("you must supply a local file to read from");

        Path outputFilename(path);
        if (outputFilename.size() == 0)
            error("you must supply a destination path to write to");

        auto data = Bytes::readFromFile(inputFilename);
        auto filesystem = Filesystem::createFilesystemFromConfig();
        filesystem->putFile(outputFilename, data);
        filesystem->flushChanges();
    }
    catch (const FilesystemException& e)
    {
        error("{}", e.message);
    }

    return 0;
}
