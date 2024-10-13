#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/config/proto.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class D64ImageReader : public ImageReader
{
public:
    D64ImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage() override
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            error("cannot open input file");

        inputFile.seekg(0, inputFile.beg);
        uint32_t begin = inputFile.tellg();
        inputFile.seekg(0, inputFile.end);
        uint32_t end = inputFile.tellg();
        uint32_t inputFileSize = (end - begin);
        inputFile.seekg(0, inputFile.beg);
        Bytes data;
        data.writer() += inputFile;
        ByteReader br(data);

        unsigned numTracks = 39;
        unsigned numHeads = 1;
        unsigned numSectors = 0;

        log("D64: reading image with {} tracks, {} heads", numTracks, numHeads);

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
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < numSectors; sectorId++)
                {
                    const auto& sector = image->put(track, head, sectorId);
                    if ((offset < inputFileSize))
                    { // still data available sector OK
                        br.seek(offset);
                        Bytes payload = br.read(256);
                        offset += 256;

                        sector->status = Sector::OK;
                        sector->data.writer().append(payload);
                    }
                    else
                    { // no more data in input file. Write sectors with status:
                      // DATA_MISSING
                        sector->status = Sector::DATA_MISSING;
                    }
                }
            }
        }

        image->calculateSize();
        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createD64ImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new D64ImageReader(config));
}
