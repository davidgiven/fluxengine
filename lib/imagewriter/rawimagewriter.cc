#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/data/flux.h"
#include "lib/core/logger.h"
#include "lib/imagewriter/imagewriter.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class RawImageWriter : public ImageWriter
{
public:
    RawImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image) override
    {
        const Geometry& geometry = image.getGeometry();

        size_t trackSize = geometry.numSectors * geometry.sectorSize;

        if (geometry.numTracks * trackSize == 0)
        {
            log("RAW: no sectors in output; skipping image file generation.");
            return;
        }

        log("RAW: writing {} tracks, {} sides",
            geometry.numTracks,
            geometry.numSides);

        std::ofstream outputFile(
            _config.filename(), std::ios::out | std::ios::binary);
        if (!outputFile.is_open())
            error("RAW: cannot open output file");

        unsigned sectorFileOffset;
        for (int track = 0; track < geometry.numTracks * geometry.numSides;
             track++)
        {
            int side = (track < geometry.numTracks) ? 0 : 1;

            std::vector<std::shared_ptr<Record>> records;
            for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
            {
                const auto& sector =
                    image.get(track % geometry.numTracks, side, sectorId);
                if (sector)
                    records.insert(records.end(),
                        sector->records.begin(),
                        sector->records.end());
            }

            std::sort(records.begin(),
                records.end(),
                [&](std::shared_ptr<Record> left, std::shared_ptr<Record> right)
                {
                    return left->startTime < right->startTime;
                });

            for (const auto& record : records)
            {
                record->rawData.writeTo(outputFile);
                Bytes(3).writeTo(outputFile);
            }
            Bytes(1).writeTo(outputFile);
        }
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createRawImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new RawImageWriter(config));
}
