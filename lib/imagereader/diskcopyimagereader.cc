#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class DiskCopyImageReader : public ImageReader
{
public:
	DiskCopyImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	std::unique_ptr<Image> readImageImpl()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

		Bytes data;
		data.writer() += inputFile;
		ByteReader br(data);

		br.seek(1);
		std::string label = br.read(data[0]);

		br.seek(0x40);
		uint32_t dataSize = br.read_be32();

		br.seek(0x50);
		uint8_t encoding = br.read_8();
		uint8_t formatByte = br.read_8();

		unsigned numCylinders = 80;
		unsigned numHeads = 2;
		unsigned numSectors = 0;
		bool mfm = false;

		switch (encoding)
		{
			case 0: /* GCR CLV 400kB */
				numHeads = 1;
				break;

			case 1: /* GCR CLV 800kB */
				break;

			case 2: /* MFM CAV 720kB */
				numSectors = 9;
				mfm = true;
				break;

			case 3: /* MFM CAV 1440kB */
				numSectors = 18;
				mfm = true;
				break;

			default:
				Error() << fmt::format("don't understand DiskCopy disks of type {}", encoding);
		}

		std::cout << "reading DiskCopy 4.2 image\n"
		          << fmt::format("{} cylinders, {} heads; {}; {}\n",
				  		numCylinders, numHeads,
						mfm ? "MFM" : "GCR",
						label);

		auto sectorsPerTrack = [&](int track) -> int
		{
			if (mfm)
				return numSectors;

			if (track < 16)
				return 12;
			if (track < 32)
				return 11;
			if (track < 48)
				return 10;
			if (track < 64)
				return 9;
			return 8;
		};

		uint32_t dataPtr = 0x54;
		uint32_t tagPtr = dataPtr + dataSize;

        std::unique_ptr<Image> image(new Image);
        for (int track = 0; track < numCylinders; track++)
        {
			int numSectors = sectorsPerTrack(track);
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
					br.seek(dataPtr);
					Bytes payload = br.read(512);
					dataPtr += 512;

					br.seek(tagPtr);
					Bytes tag = br.read(12);
					tagPtr += 12;

                    const auto& sector = image->put(track, head, sectorId);
                    sector->status = Sector::OK;
                    sector->logicalTrack = sector->physicalCylinder = track;
                    sector->logicalSide = sector->physicalHead = head;
                    sector->logicalSector = sectorId;
                    sector->data.writer().append(payload).append(tag);
                }
            }
        }

		image->setGeometry({
			.numTracks = numCylinders,
			.numSides = numHeads,
			.numSectors = 12,
			.sectorSize = 512 + 12,
			.irregular = true
		});
        return image;
	}
};

std::unique_ptr<ImageReader> ImageReader::createDiskCopyImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new DiskCopyImageReader(config));
}


