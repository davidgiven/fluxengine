#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"

static FlagGroup flags;

int mainErase(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

	Error() << "TODO";

	#if 0
	writeTracks(
        [](int physicalTrack, int physicalSide) -> std::unique_ptr<Fluxmap>
        {
            return std::unique_ptr<Fluxmap>();
        }
    );
	#endif

    return 0;
}

