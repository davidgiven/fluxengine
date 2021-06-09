#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "decoders/decoders.h"
#include "arch/northstar/northstar.h"
#include "lib/imagewriter/imagewriter.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class NsiImageWriter : public ImageWriter
{
public:
	NsiImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{}

	void writeImage(const SectorSet& sectors)
	{
		unsigned autoTracks;
		unsigned autoSides;
		unsigned autoSectors;
		unsigned autoBytes;
		sectors.calculateSize(autoTracks, autoSides, autoSectors, autoBytes);

		size_t trackSize = autoSectors * autoBytes;

		std::cout << fmt::format("Writing {} cylinders, {} heads, {} sectors, {} ({} bytes/sector), {} kB total",
				autoTracks, autoSides,
				autoSectors, autoBytes == 256 ? "SD" : "DD", autoBytes,
				autoTracks * trackSize / 1024)
				<< std::endl;

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		unsigned sectorFileOffset;
		for (int track = 0; track < autoTracks * autoSides; track++)
		{
			int head = (track < autoTracks) ? 0 : 1;
			for (int sectorId = 0; sectorId < autoSectors; sectorId++)
			{
				const auto& sector = sectors.get(track % autoTracks, head, sectorId);
				if (sector)
				{
					if (head == 0) { /* Side 0 is from track 0-34 */
						sectorFileOffset = track * trackSize + sectorId * autoBytes;
					}
					else { /* Side 1 is from track 70-35 */
						sectorFileOffset = (autoBytes * autoSectors * autoTracks) + /* Skip over side 0 */
							((autoTracks - 1) - (track % autoTracks)) * (autoBytes * autoSectors) +
							(sectorId * autoBytes); /* Sector offset from beginning of track. */
					}
					outputFile.seekp(sectorFileOffset, std::ios::beg);
					sector->data.slice(0, autoBytes).writeTo(outputFile);
				}
			}
		}
	}

	void putBlock(size_t offset, size_t length, const Bytes& data)
	{ throw "unimplemented"; }
};

std::unique_ptr<ImageWriter> ImageWriter::createNsiImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new NsiImageWriter(config));
}
