#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "image.h"
#include "ldbs.h"
#include "logger.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static int sectors_per_track(int track)
{
    if (track < 17)
        return 21;
    if (track < 24)
        return 19;
    if (track < 30)
        return 18;
    return 17;
}

class D64ImageWriter : public ImageWriter
{
public:
    D64ImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image)
    {
        log("D64: writing triangular image");

        std::ofstream outputFile(
            _config.filename(), std::ios::out | std::ios::binary);
        if (!outputFile.is_open())
            error("cannot open output file");

        uint32_t offset = 0;
        for (int track = 0; track < 40; track++)
        {
            int sectorCount = sectors_per_track(track);
            for (int sectorId = 0; sectorId < sectorCount; sectorId++)
            {
                const auto& sector = image.get(track, 0, sectorId);
                if (sector)
                {
                    outputFile.seekp(offset);
                    outputFile.write((const char*)sector->data.cbegin(),
                        sector->data.size());
                }

                offset += 256;
            }
        }
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createD64ImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new D64ImageWriter(config));
}
