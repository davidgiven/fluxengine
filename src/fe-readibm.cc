#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include <fmt/format.h>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "ibm.img");

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

    std::vector<std::unique_ptr<Sector>> allSectors;
    for (auto& track : readTracks())
    {
        Fluxmap& fluxmap = track->read();

        nanoseconds_t clockPeriod = fluxmap.guessClock();
        std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

        /* For MFM, the bit clock is half the detected clock. */
        auto bitmap = decodeFluxmapToBits(fluxmap, clockPeriod/2);
        std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

        auto records = decodeBitsToRecordsMfm(bitmap);
        std::cout << records.size() << " records." << std::endl;

        auto sectors = decodeIbmRecordsToSectors(records);
        std::cout << "       " << sectors.size() << " sectors; ";

        int size = 0;
        for (auto& sector : sectors)
        {
            size += sector->data().size();
            allSectors.push_back(std::move(sector));
        }
        std::cout << size << " bytes decoded." << std::endl;
    }

    writeSectorsToFile(allSectors, outputFilename);
    return 0;
}

