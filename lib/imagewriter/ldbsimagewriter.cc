#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "ldbs.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class LDBSImageWriter : public ImageWriter
{
public:
	LDBSImageWriter(const SectorSet& sectors, const ImageSpec& spec):
		ImageWriter(sectors, spec)
	{}

	void writeImage()
	{
        LDBS ldbs;

		unsigned numCylinders = spec.cylinders;
		unsigned numHeads = spec.heads;
		unsigned numSectors = spec.sectors;
		unsigned numBytes = spec.bytes;
		std::cout << fmt::format("writing {} tracks, {} heads, {} sectors, {} bytes per sector",
						numCylinders, numHeads,
						numSectors, numBytes)
				<< std::endl;

        Bytes trackDirectory;
        ByteWriter trackDirectoryWriter(trackDirectory);
        int trackDirectorySize = 0;
        trackDirectoryWriter.write_le16(0);

		for (int track = 0; track < numCylinders; track++)
		{
			for (int head = 0; head < numHeads; head++)
			{
                Bytes trackHeader;
                ByteWriter trackHeaderWriter(trackHeader);

                int actualSectors = 0;
				for (int sectorId = 0; sectorId < numSectors; sectorId++)
				{
					const auto& sector = sectors.get(track, head, sectorId);
					if (sector)
                        actualSectors++;
                }

                trackHeaderWriter.write_le16(0x000C); /* offset of sector headers */
                trackHeaderWriter.write_le16(0x0012); /* length of each sector descriptor */
                trackHeaderWriter.write_le16(actualSectors);
                trackHeaderWriter.write_8(0); /* data rate unknown */
                trackHeaderWriter.write_8(0); /* recording mode unknown */
                trackHeaderWriter.write_8(0); /* format gap length */
                trackHeaderWriter.write_8(0); /* filler byte */
                trackHeaderWriter.write_le16(0); /* approximate track length */

				for (int sectorId = 0; sectorId < numSectors; sectorId++)
				{
					const auto& sector = sectors.get(track, head, sectorId);
					if (sector)
					{
                        uint32_t sectorLabel = (('S') << 24) | ((track & 0xff) << 16) | (head << 8) | sectorId;
                        uint32_t sectorAddress = ldbs.put(sector->data, sectorLabel);

                        trackHeaderWriter.write_8(track);
                        trackHeaderWriter.write_8(head);
                        trackHeaderWriter.write_8(sectorId);
                        trackHeaderWriter.write_8(0); /* power-of-two size */
                        trackHeaderWriter.write_8((sector->status == Sector::OK) ? 0x00 : 0x20); /* 8272 status 1 */
                        trackHeaderWriter.write_8(0); /* 8272 status 2 */
                        trackHeaderWriter.write_8(1); /* number of copies */
                        trackHeaderWriter.write_8(0); /* filler byte */
                        trackHeaderWriter.write_le32(sectorAddress);
                        trackHeaderWriter.write_le16(0); /* trailing bytes */
                        trackHeaderWriter.write_le16(0); /* approximate offset */
                        trackHeaderWriter.write_le16(sector->data.size());
					}
				}

                uint32_t trackLabel = (('T') << 24) | ((track & 0xff) << 16) | ((track >> 8) << 8) | head;
                uint32_t trackHeaderAddress = ldbs.put(trackHeader, trackLabel);
                trackDirectoryWriter.write_be32(trackLabel);
                trackDirectoryWriter.write_le32(trackHeaderAddress);
                trackDirectorySize++;
			}
		}

        trackDirectoryWriter.seek(0);
        trackDirectoryWriter.write_le16(trackDirectorySize);

        uint32_t trackDirectoryAddress = ldbs.put(trackDirectory, LDBS_TRACK_BLOCK);
        Bytes data = ldbs.write(trackDirectoryAddress);
        data.writeToFile(spec.filename);
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createLDBSImageWriter(
	const SectorSet& sectors, const ImageSpec& spec)
{
    return std::unique_ptr<ImageWriter>(new LDBSImageWriter(sectors, spec));
}
