#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "writer.h"
#include <fmt/format.h>
#include <fstream>
#include <ctype.h>

int main(int argc, const char* argv[])
{
    setReaderDefaultSource(":t=0-81:h=0-1");
    setWriterDefaultDest(":t=0-81:s=0-1");
    Flag::parseFlags(argc, argv);

    auto tracks = readTracks();
    for (auto& track : tracks)
        track->read();

    writeTracks(
        [&](unsigned physicalTrack, unsigned physicalSide) -> std::unique_ptr<Fluxmap>
        {
            for (auto& track : tracks)
            {
                if (track && (track->track == physicalTrack) && (track->side == physicalSide))
                    return track->read();
            }
            Error() << "missing in source";
            throw 0; /* unreachable */
        }
    );

    return 0;
}

