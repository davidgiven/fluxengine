#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);
    for (auto& track : readTracks())
    {
        track->read();
        std::cout << "track " << track->track << " " << track->side << std::endl;

    }
    return 0;
}

