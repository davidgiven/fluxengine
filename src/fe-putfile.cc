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
			Error() << "you must supply a local file to read from";

        Path outputFilename(path);
        if (outputFilename.size() == 0)
            Error() << "you must supply a destination path to write to";

		auto data = Bytes::readFromFile(inputFilename);
        auto filesystem = createFilesystemFromConfig();
        filesystem->putFile(outputFilename, data);
		filesystem->flush();
    }
    catch (const FilesystemException& e)
    {
        Error() << e.message;
    }

    return 0;
}
