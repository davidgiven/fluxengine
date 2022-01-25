#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include "image.h"
#include "proto.h"
#include <algorithm>
#include <iostream>
#include <fstream>

enum {
    SIZE_35T = 683*256,
    SIZE_35T_ERRORS = 683*257,
    SIZE_40T = 768*256,
    SIZE_40T_ERRORS = 768*257,
};


class D64ImageReader : public ImageReader
{
public:
	D64ImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	std::unique_ptr<Image> readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        inputFile.seekg(0, inputFile.beg);
        uint32_t begin = inputFile.tellg();
        inputFile.seekg(0, inputFile.end);
        uint32_t end = inputFile.tellg();
        uint32_t inputFileSize = (end-begin);
        inputFile.seekg(0, inputFile.beg);
		Bytes data;
		data.writer() += inputFile;
		ByteReader br(data);

		unsigned numCylinders = 39;
		unsigned numHeads = 1;
		unsigned numSectors = 0;
        bool errors = false;

        switch(inputFileSize) {
            case SIZE_35T_ERRORS:
                errors = true;
                [[fallthrough]];
            case SIZE_35T:
                numCylinders = 34;
                break;
            case SIZE_40T_ERRORS:
                errors = true;
                [[fallthrough]];
            case SIZE_40T:
                break;

            default:
                Error() << fmt::format("Invalid d64 file size {} bytes",
                            inputFileSize);
        }
		std::cout << "reading D64 image\n"
		          << fmt::format("{} cylinders, {} heads\n",
				  		numCylinders, numHeads);

        uint32_t offset = 0;

		auto sectorsPerTrack = [&](int track) -> int
		{
            if (track < 17)
                return 21;
            if (track < 24)
                return 19;
            if (track < 30)
                return 18;
            return 17;
		};

        std::unique_ptr<Image> image(new Image);
        for (int track = 0; track < 40; track++)
        {
			int numSectors = sectorsPerTrack(track);
			int physicalCylinder = track*2;
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
					const auto& sector = image->put(track, head, sectorId);
                    if ((offset < inputFileSize))
                    {    //still data available sector OK
						br.seek(offset);
                        Bytes payload = br.read(256);
                        offset += 256;

                        sector->status = Sector::OK;
                        sector->logicalTrack = track;
						sector->physicalCylinder = physicalCylinder;
                        sector->logicalSide = sector->physicalHead = head;
                        sector->logicalSector = sectorId;
                        sector->data.writer().append(payload);
                    }
					else
                    {   //no more data in input file. Write sectors with status: DATA_MISSING
                        sector->status = Sector::DATA_MISSING;
                        sector->logicalTrack = track;
						sector->physicalCylinder = physicalCylinder;
                        sector->logicalSide = sector->physicalHead = head;
                        sector->logicalSector = sectorId;
                    }
                }
            }
        }

		image->calculateSize();
        return image;
	}
};

std::unique_ptr<ImageReader> ImageReader::createD64ImageReader(const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D64ImageReader(config));
}


