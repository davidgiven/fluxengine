#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/config/config.pb.h"
#include "lib/data/layout.h"
#include "lib/core/logger.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static int countl_zero(uint32_t value)
{
    int count = 0;
    while (!(value & 0x80000000))
    {
        value <<= 1;
        count++;
    }
    return count;
}

class D88ImageWriter : public ImageWriter
{
public:
    D88ImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image) override
    {
        const Geometry geometry = image.getGeometry();

        int tracks = geometry.numTracks;
        int sides = geometry.numSides;

        std::ofstream outputFile(
            _config.filename(), std::ios::out | std::ios::binary);
        if (!outputFile.is_open())
            error("cannot open output file");

        Bytes header;
        ByteWriter headerWriter(header);
        for (int i = 0; i < 26; i++)
        {
            headerWriter.write_8(0x0); // image name + reserved bytes
        }
        headerWriter.write_8(0x00); // not write protected
        if (geometry.numTracks > 42)
        {
            headerWriter.write_8(0x20); // 2HD
        }
        else
        {
            headerWriter.write_8(0x00); // 2D
        }
        headerWriter.write_le32(
            0); // disk size (will be overridden at the end of writing)
        for (int i = 0; i < 164; i++)
        {
            headerWriter.write_le32(
                0); // track pointer (will be overridden in track loop)
        }
        header.writeTo(outputFile);

        uint32_t trackOffset = 688;

        for (int track = 0; track < geometry.numTracks * geometry.numSides;
             track++)
        {
            headerWriter.seek(0x20 + 4 * track);
            headerWriter.write_le32(trackOffset);
            int side = track & 1;
            std::vector<std::shared_ptr<const Sector>> sectors;
            for (int sectorId = geometry.firstSector;
                 sectorId <= geometry.numSectors;
                 sectorId++)
            {
                const auto& sector = image.get(track >> 1, side, sectorId);
                if (sector)
                {
                    sectors.push_back(sector);
                }
            }
            std::sort(begin(sectors),
                end(sectors),
                [](std::shared_ptr<const Sector> a,
                    std::shared_ptr<const Sector> b)
                {
                    return a->position < b->position;
                });
            for (auto& sector : sectors)
            {
                Bytes sectorBytes;
                ByteWriter sectorWriter(sectorBytes);
                sectorWriter.write_8(sector->logicalTrack);
                sectorWriter.write_8(sector->logicalSide);
                sectorWriter.write_8(sector->logicalSector);
                sectorWriter.write_8(
                    24 - countl_zero(uint32_t(sector->data.size())));
                sectorWriter.write_le16(sectors.size());
                sectorWriter.write_8(0x00); // always write mfm
                sectorWriter.write_8(0x00); // always write not deleted data
                if (sector->status == Sector::Status::BAD_CHECKSUM)
                {
                    sectorWriter.write_8(0xB0);
                }
                else
                {
                    sectorWriter.write_8(0x00);
                }
                sectorWriter.write_8(0x00); // reserved
                sectorWriter.write_8(0x00);
                sectorWriter.write_8(0x00);
                sectorWriter.write_8(0x00);
                sectorWriter.write_8(0x00);
                sectorWriter.write_le16(sector->data.size());
                sectorBytes.writeTo(outputFile);
                sector->data.writeTo(outputFile);
                trackOffset += sectorBytes.size();
                trackOffset += sector->data.size();
            }
        }

        headerWriter.seek(0x1c);
        headerWriter.write_le32(outputFile.tellp());
        outputFile.seekp(0);
        header.writeTo(outputFile);

        log("D88: wrote {} tracks, {} sides, {} kB total",
            tracks,
            sides,
            outputFile.tellp() / 1024);
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createD88ImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new D88ImageWriter(config));
}
