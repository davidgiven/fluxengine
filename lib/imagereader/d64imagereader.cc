#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include "proto.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class D64ImageReader : public ImageReader
{
public:
	D64ImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	Bytes getBlock(size_t offset, size_t length) const
	{}

	SectorSet readImage()
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

        SectorSet sectors;
        for (int track = 0; track < 40; track++)
        {
			int numSectors = sectorsPerTrack(track);
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
                   if ((offset < inputFileSize))
                   {    //still data available sector OK
                        br.seek(offset);
                        Bytes payload = br.read(256);
                        offset += 256;

                        std::unique_ptr<Sector>& sector = sectors.get(track, head, sectorId);
                        sector.reset(new Sector);
                        sector->status = Sector::OK;
                        sector->logicalTrack = sector->physicalTrack = track;
                        sector->logicalSide = sector->physicalSide = head;
                        sector->logicalSector = sectorId;
                        sector->data.writer().append(payload);
                    } else
                    {   //no more data in input file. Write sectors with status: DATA_MISSING
                        std::unique_ptr<Sector>& sector = sectors.get(track, head, sectorId);
                        sector.reset(new Sector);
                        sector->status = Sector::DATA_MISSING;
                        sector->logicalTrack = sector->physicalTrack = track;
                        sector->logicalSide = sector->physicalSide = head;
                        sector->logicalSector = sectorId;
                    }
                }
            }
        }
        return sectors;
	}
};

std::unique_ptr<ImageReader> ImageReader::createD64ImageReader(const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D64ImageReader(config));
}


