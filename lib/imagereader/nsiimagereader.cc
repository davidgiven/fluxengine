/* Image reader for Northstar floppy disk images */

#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "fmt/format.h"
#include "lib/imagereader/imagereader.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class NsiImageReader : public ImageReader
{
public:
	NsiImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	Image readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

		const auto begin = inputFile.tellg();
		inputFile.seekg(0, std::ios::end);
		const auto end = inputFile.tellg();
		const auto fsize = (end - begin);

		std::cout << "NSI: Autodetecting geometry based on file size: " << fsize << std::endl;

		unsigned numCylinders = 35;
		unsigned numSectors = 10;
		unsigned numHeads = 2;
		unsigned sectorSize = 512;

		switch (fsize) {
			case 358400:
				numHeads = 2;
				sectorSize = 512;
				break;

			case 179200:
				numHeads = 1;
				sectorSize = 512;
				break;
				
			case 89600:
				numHeads = 1;
				sectorSize = 256;
				break;

			default:
				Error() << "NSI: unknown file size";
		}

        size_t trackSize = numSectors * sectorSize;

        std::cout << fmt::format("reading {} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
                        numCylinders, numHeads,
                        numSectors, sectorSize,
                        numCylinders * numHeads * trackSize / 1024)
                << std::endl;

        Image image;
        unsigned sectorFileOffset;

        for (unsigned head = 0; head < numHeads; head++)
        {
            for (unsigned track = 0; track < numCylinders; track++)
            {
                for (unsigned sectorId = 0; sectorId < numSectors; sectorId++)
                {
                    if (head == 0) { /* Head 0 is from track 0-34 */
                        sectorFileOffset = track * trackSize + sectorId * sectorSize;
                    }
                    else { /* Head 1 is from track 70-35 */
                        sectorFileOffset = (trackSize * numCylinders) + /* Skip over side 0 */
                            ((numCylinders - track - 1) * trackSize) +
                            (sectorId * sectorSize); /* Sector offset from beginning of track. */
                    }

                    inputFile.seekg(sectorFileOffset, std::ios::beg);

                    Bytes data(sectorSize);
                    inputFile.read((char*) data.begin(), sectorSize);

                    const auto& sector = image.put(track, head, sectorId);
                    sector->status = Sector::OK;
                    sector->logicalTrack = sector->physicalCylinder = track;
                    sector->logicalSide = sector->physicalHead = head;
                    sector->logicalSector = sectorId;
                    sector->data = data;
                }
            }
        }

		Geometry g;
		g.numTracks = numCylinders;
		g.numSides = numHeads;
		g.numSectors = numSectors;
		g.sectorSize = sectorSize;
		image.setGeometry(g);
        return image;
	}
};

std::unique_ptr<ImageReader> ImageReader::createNsiImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new NsiImageReader(config));
}

