#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "geometry/geometry.h"
#include "fmt/format.h"
#include "ldbs.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class LDBSImageWriter : public ImageWriter, public AssemblingGeometryMapper
{
public:
	LDBSImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{
	}

	~LDBSImageWriter()
	{
        LDBS ldbs;

		unsigned numCylinders;
		unsigned numHeads;
		unsigned numSectors;
		unsigned numBytes;
		_sectors.calculateSize(numCylinders, numHeads, numSectors, numBytes);

		std::cout << fmt::format("LDBS: writing {} tracks, {} heads, {} sectors, {} bytes per sector",
						numCylinders, numHeads,
						numSectors, numBytes)
				<< std::endl;

		Bytes geomBlock;
		ByteWriter geomBlockWriter(geomBlock);
		geomBlockWriter.write_8(0); /* alternating sides */
		geomBlockWriter.write_le16(numCylinders);
		geomBlockWriter.write_8(numHeads);
		geomBlockWriter.write_8(numSectors);
		geomBlockWriter.write_8(0); /* first sector ID */
		geomBlockWriter.write_le16(numBytes);
		geomBlockWriter.write_8(0); /* data rate */
		geomBlockWriter.write_8(0); /* read/write gap */
		geomBlockWriter.write_8(0); /* format gap */
		geomBlockWriter.write_8(0); /* recording mode */
		geomBlockWriter.write_8(0); /* complement flag */
		geomBlockWriter.write_8(0); /* disable multitrack read/writes */
		geomBlockWriter.write_8(0); /* do not skip deleted data */

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
					const auto& sector = _sectors.get(track, head, sectorId);
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
					const auto& sector = _sectors.get(track, head, sectorId);
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

		trackDirectoryWriter.write_be32(LDBS_GEOM_BLOCK);
		trackDirectoryWriter.write_le32(ldbs.put(geomBlock, LDBS_GEOM_BLOCK));
		trackDirectorySize++;

        trackDirectoryWriter.seek(0);
        trackDirectoryWriter.write_le16(trackDirectorySize);

        uint32_t trackDirectoryAddress = ldbs.put(trackDirectory, LDBS_TRACK_BLOCK);
        Bytes data = ldbs.write(trackDirectoryAddress);
        data.writeToFile(_config.filename());

        std::cout << fmt::format("LDBS: written output image of {} kB total\n",
						data.size() / 1024);
	}

	const AssemblingGeometryMapper* getGeometryMapper() const
	{
		return this;
	}

	void put(const Sector& sector) const
	{
		auto& ptr = _sectors.get(sector.logicalTrack, sector.logicalSide, sector.logicalSector);
		ptr.reset(new Sector(sector));
	}

	void putBlock(size_t offset, size_t length, const Bytes& data)
	{ throw "unimplemented"; }

private:
	mutable SectorSet _sectors;
};

std::unique_ptr<ImageWriter> ImageWriter::createLDBSImageWriter(const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new LDBSImageWriter(config));
}
