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
    setReaderDefaults(0, 81, 0, 1);
    setWriterDefaults(0, 81, 0, 1);
    Flag::parseFlags(argc, argv);

    auto tracks = readTracks();
    for (auto& track : tracks)
        track->read();

    writeTracks(
        0, 81,
        [&](int physicalTrack, int physicalSide) -> Fluxmap
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

