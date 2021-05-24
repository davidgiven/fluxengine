/* Image reader for Northstar floppy disk images */

#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class NsiImageReader : public ImageReader
{
public:
	NsiImageReader(const ImageSpec& spec):
		ImageReader(spec)
	{}

	SectorSet readImage()
	{
        std::ifstream inputFile(spec.filename, std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        if ((spec.cylinders == 0) && (spec.heads == 0) && (spec.sectors == 0) && (spec.bytes == 0)) {
            const auto begin = inputFile.tellg();
            inputFile.seekg(0, std::ios::end);
            const auto end = inputFile.tellg();
            const auto fsize = (end - begin);

            std::cout << "NSI: Autodetecting geometry based on file size: " << fsize << std::endl;

            spec.cylinders = 35;
            spec.sectors = 10;

            switch (fsize) {
            case 358400:
                spec.heads = 2;
                spec.bytes = 512;
                break;
            case 179200:
                spec.heads = 1;
                spec.bytes = 512;
                break;
            case 89600:
                spec.heads = 1;
                spec.bytes = 256;
                break;
            }
        }

        size_t trackSize = spec.sectors * spec.bytes;

        std::cout << fmt::format("reading {} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
                        spec.cylinders, spec.heads,
                        spec.sectors, spec.bytes,
                        spec.cylinders * spec.heads * trackSize / 1024)
                << std::endl;

        SectorSet sectors;
        unsigned sectorFileOffset;

        for (int head = 0; head < spec.heads; head++)
        {
            for (int track = 0; track < spec.cylinders; track++)
            {
                for (int sectorId = 0; sectorId < spec.sectors; sectorId++)
                {
                    if (head == 0) { /* Head 0 is from track 0-34 */
                        sectorFileOffset = track * trackSize + sectorId * spec.bytes;
                    }
                    else { /* Head 1 is from track 70-35 */
                        sectorFileOffset = (trackSize * spec.cylinders) + /* Skip over side 0 */
                            ((spec.cylinders - track - 1) * trackSize) +
                            (sectorId * spec.bytes); /* Sector offset from beginning of track. */
                    }

                    inputFile.seekg(sectorFileOffset, std::ios::beg);

                    Bytes data(spec.bytes);
                    inputFile.read((char*) data.begin(), spec.bytes);

                    std::unique_ptr<Sector>& sector = sectors.get(track, head, sectorId);
                    sector.reset(new Sector);
                    sector->status = Sector::OK;
                    sector->logicalTrack = sector->physicalTrack = track;
                    sector->logicalSide = sector->physicalSide = head;
                    sector->logicalSector = sectorId;
                    sector->data = data;
                }
            }
        }
        return sectors;
	}
};

std::unique_ptr<ImageReader> ImageReader::createNsiImageReader(
	const ImageSpec& spec)
{
    return std::unique_ptr<ImageReader>(new NsiImageReader(spec));
}

