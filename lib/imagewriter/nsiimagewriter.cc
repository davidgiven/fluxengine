#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "decoders/decoders.h"
#include "arch/northstar/northstar.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class NSIImageWriter : public ImageWriter
{
public:
	NSIImageWriter(const SectorSet& sectors, const ImageSpec& spec):
		ImageWriter(sectors, spec)
	{}

	void writeImage()
	{
		unsigned numCylinders = spec.cylinders;
		unsigned numHeads = spec.heads;
		unsigned numSectors = spec.sectors;
		unsigned numTracks = numCylinders * numHeads;
		unsigned numBytes = spec.bytes;
		int head;

		size_t trackSize = numSectors * numBytes;

		if ((numBytes != 256) && (numBytes != 512) && (numBytes != 257) && (numBytes != 513))
			Error() << "Sector size must be 256 or 512.";

		if (numCylinders != 35)
			Error() << "Number of cylinders must be 35.";

		std::cout << fmt::format("Writing {} cylinders, {} heads, {} sectors, {} ({} bytes/sector), {} kB total",
				numCylinders, numHeads,
				numSectors, numBytes == 256 ? "SD" : "DD", numBytes,
				numTracks * trackSize / 1024)
				<< std::endl;

		std::ofstream outputFile(spec.filename, std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		unsigned sectorFileOffset;

		for (int track = 0; track < numCylinders * numHeads; track++)
		{
			head = (track < numCylinders) ? 0 : 1;
			for (int sectorId = 0; sectorId < numSectors; sectorId++)
			{
				const auto& sector = sectors.get(track % numCylinders, head, sectorId);
				if (sector)
				{
					if (head == 0) { /* Side 0 is from track 0-34 */
						sectorFileOffset = track * trackSize + sectorId * numBytes;
					}
					else { /* Side 1 is from track 70-35 */
						sectorFileOffset = (numBytes * numSectors * numCylinders) + /* Skip over side 0 */
							((numCylinders - 1) - (track % numCylinders)) * (numBytes * numSectors) +
							(sectorId * numBytes); /* Sector offset from beginning of track. */
					}
					outputFile.seekp(sectorFileOffset, std::ios::beg);
					sector->data.slice(0, numBytes).writeTo(outputFile);
				}
			}
		}
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createNSIImageWriter(
	const SectorSet& sectors, const ImageSpec& spec)
{
    return std::unique_ptr<ImageWriter>(new NSIImageWriter(sectors, spec));
}
