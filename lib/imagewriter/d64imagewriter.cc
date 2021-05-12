#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "ldbs.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static int sectors_per_track(int track)
{
    if (track < 17)
        return 21;
    if (track < 24)
        return 19;
    if (track < 30)
        return 18;
    return 17;
}

class D64ImageWriter : public ImageWriter
{
public:
	D64ImageWriter(const Config_OutputFile& config):
		ImageWriter(config)
	{}

	void writeImage(const SectorSet& sectors)
	{
		std::cout << "writing D64 triangular image\n";

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

        uint32_t offset = 0;
		for (int track = 0; track < 40; track++)
		{
            int sectorCount = sectors_per_track(track);
            for (int sectorId = 0; sectorId < sectorCount; sectorId++)
            {
                const auto& sector = sectors.get(track, 0, sectorId);
                if (sector)
                {
                    outputFile.seekp(offset);
                    outputFile.write((const char*) sector->data.cbegin(), 256);
                }

                offset += 256;
            }
		}
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createD64ImageWriter(const Config_OutputFile& config)
{
    return std::unique_ptr<ImageWriter>(new D64ImageWriter(config));
}

