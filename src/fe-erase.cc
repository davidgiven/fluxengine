#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"

FlagGroup flags;

int main(int argc, const char* argv[])
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

