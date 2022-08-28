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

static FlagGroup flags({&fileFlags});

static StringFlag directory({"-p", "--path"}, "path to work on", "");
static StringFlag output({"-o", "--output"}, "local filename to write to", "");

int mainGetFile(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("getfile", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        Path inputFilename(directory);
        if (inputFilename.size() == 0)
            Error() << "you must supply a filename to read";

        std::string outputFilename = output;
        if (outputFilename.empty())
            outputFilename = inputFilename.back();

        auto filesystem = createFilesystemFromConfig();
        auto data = filesystem->getFile(inputFilename);
        data.writeToFile(outputFilename);
    }
    catch (const FilesystemException& e)
    {
        Error() << e.message;
    }

    return 0;
}
