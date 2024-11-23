#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/external/ldbs.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/config/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static const char LABEL[] = "FluxEngine image";

static void write_and_update_checksum(
    ByteWriter& bw, uint32_t& checksum, const Bytes& data)
{
    ByteReader br(data);
    while (!br.eof())
    {
        uint32_t i = br.read_be16();
        checksum += i;
        checksum = (checksum >> 1) | (checksum << 31);
        bw.write_be16(i);
    }
}

class DiskCopyImageWriter : public ImageWriter
{
public:
    DiskCopyImageWriter(const ImageWriterProto& config): ImageWriter(config) {}

    void writeImage(const Image& image) override
    {
        const Geometry& geometry = image.getGeometry();

        bool mfm = false;

        switch (geometry.sectorSize)
        {
            case 524:
                /* GCR disk */
                break;

            case 512:
                /* MFM disk */
                mfm = true;
                break;

            default:
                error(
                    "this image is not compatible with the DiskCopy 4.2 "
                    "format");
        }

        log("DC42: writing DiskCopy 4.2 image");
        log("DC42: {} tracks, {} sides, {} sectors, {} bytes per sector; {}",
            geometry.numTracks,
            geometry.numSides,
            geometry.numSectors,
            geometry.sectorSize,
            mfm ? "MFM" : "GCR");

        auto sectors_per_track = [&](int track) -> int
        {
            if (mfm)
                return geometry.numSectors;

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

        Bytes data;
        ByteWriter bw(data);

        /* Write the actual sectr data. */

        uint32_t dataChecksum = 0;
        uint32_t tagChecksum = 0;
        uint32_t offset = 0x54;
        uint32_t sectorDataStart = offset;
        for (int track = 0; track < geometry.numTracks; track++)
        {
            for (int side = 0; side < geometry.numSides; side++)
            {
                int sectorCount = sectors_per_track(track);
                for (int sectorId = 0; sectorId < sectorCount; sectorId++)
                {
                    const auto& sector = image.get(track, side, sectorId);
                    if (sector)
                    {
                        bw.seek(offset);
                        write_and_update_checksum(
                            bw, dataChecksum, sector->data.slice(0, 512));
                    }
                    offset += 512;
                }
            }
        }
        uint32_t sectorDataEnd = offset;
        if (!mfm)
        {
            for (int track = 0; track < geometry.numTracks; track++)
            {
                for (int side = 0; side < geometry.numSides; side++)
                {
                    int sectorCount = sectors_per_track(track);
                    for (int sectorId = 0; sectorId < sectorCount; sectorId++)
                    {
                        const auto& sector = image.get(track, side, sectorId);
                        if (sector)
                        {
                            bw.seek(offset);
                            write_and_update_checksum(
                                bw, tagChecksum, sector->data.slice(512, 12));
                        }
                        offset += 12;
                    }
                }
            }
        }
        uint32_t tagDataEnd = offset;

        /* Write the header. */

        uint8_t encoding;
        uint8_t format;
        if (mfm)
        {
            format = 0x22;
            if (geometry.numSectors == 18)
                encoding = 3;
            else
                encoding = 2;
        }
        else
        {
            if (geometry.numSides == 2)
            {
                encoding = 1;
                format = 0x22;
            }
            else
            {
                encoding = 0;
                format = 0x02;
            }
        }

        bw.seek(0);
        bw.write_8(sizeof(LABEL));
        bw.append(LABEL);
        bw.seek(0x40);
        bw.write_be32(sectorDataEnd - sectorDataStart); /* data size */
        bw.write_be32(tagDataEnd - sectorDataEnd);      /* tag size */
        bw.write_be32(dataChecksum);                    /* data checksum */
        bw.write_be32(tagChecksum);                     /* tag checksum */
        bw.write_8(encoding);                           /* encoding */
        bw.write_8(format);                             /* format byte */
        bw.write_be16(0x0100);                          /* magic number */

        data.writeToFile(_config.filename());
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createDiskCopyImageWriter(
    const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new DiskCopyImageWriter(config));
}
