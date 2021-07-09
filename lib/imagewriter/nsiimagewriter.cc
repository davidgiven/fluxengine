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
		bool mixedDensity = false;
		sectors.calculateSize(autoTracks, autoSides, autoSectors, autoBytes);

		size_t trackSize = autoSectors * autoBytes;

		if (autoTracks * trackSize == 0) {
			std::cout << "No sectors in output; skipping .nsi image file generation." << std::endl;
			return;
		}

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
					if ((autoBytes == 512) && (sector->data.size() == 256)) {
						/* North Star DOS provided an upgrade path for disks formatted as single-
						 * density to hold double-density data without reformatting.  In this
						 * case, the four directory blocks will be single-density but other areas
						 * of the disk are double-density.  This cannot be accurately represented
						 * using a .nsi file, so in these cases, we pad the sector to 512-bytes,
						 * filling with spaces.
						 */
						char fill[256];
						memset(fill, ' ', sizeof(fill));
						if (mixedDensity == false) {
							std::cout << "Warning: Disk contains mixed single/double-density sectors." << std::endl;
						}
						mixedDensity = true;
						sector->data.slice(0, 256).writeTo(outputFile);
						outputFile.write(fill, sizeof(fill));
					} else {
						sector->data.slice(0, autoBytes).writeTo(outputFile);
					}
				}
			}
		}
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createNsiImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new NsiImageWriter(config));
}
