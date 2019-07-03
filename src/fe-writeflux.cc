#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "writer.h"
#include "record.h"
#include "sector.h"
#include "track.h"
#include "fmt/format.h"
#include <fstream>
#include <ctype.h>

static FlagGroup flags { &writerFlags };

int mainWriteFlux(int argc, const char* argv[])
{
    setReaderDefaultSource(":t=0-81:h=0-1");
    setWriterDefaultDest(":t=0-81:s=0-1");
    flags.parseFlags(argc, argv);

    auto tracks = readTracks();
    for (auto& track : tracks)
        track->readFluxmap();

    writeTracks(
        [&](unsigned physicalTrack, unsigned physicalSide) -> std::unique_ptr<Fluxmap>
        {
            for (auto& track : tracks)
            {
                if (track && (track->physicalTrack == physicalTrack) && (track->physicalSide == physicalSide))
                {
                    /* 
                     * std::move actually isn't really allowed here as it'll
                     * cause the fluxmap to be lost. But let's go with it
                     * anyway until this code gets rewritten.
                     */
                    return std::move(track->fluxmap);
                }
            }
            Error() << "missing in source";
            throw "unreachable";
        }
    );

    return 0;
}

