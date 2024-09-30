#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/proto.h"
#include "fluxengine.h"
#include "lib/vfs/vfs.h"
#include "lib/utils.h"
#include "lib/usb/usb.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

static StringFlag oldFilename({"--path1"}, "old filename", "");
static StringFlag newFilename({"--path2"}, "new filename", "");

int mainMv(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("mv", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    auto usb = USB::create();
    auto filesystem = Filesystem::createFilesystemFromConfig();

    Path oldPath(oldFilename);
    if (oldPath.size() == 0)
        error("old filename missing");

    Path newPath(newFilename);
    if (newPath.size() == 0)
        error("new filename missing");

    filesystem->moveFile(oldPath, newPath);
    filesystem->flushChanges();

    return 0;
}
