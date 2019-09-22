#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"

static FlagGroup flags { &writerFlags };

int mainErase(int argc, const char* argv[])
{
	setWriterDefaultDest(":t=0-81:s=0-1");
    flags.parseFlags(argc, argv);

	writeTracks(
        [](int physicalTrack, int physicalSide) -> std::unique_ptr<Fluxmap>
        {
            return std::unique_ptr<Fluxmap>();
        }
    );

    return 0;
}

