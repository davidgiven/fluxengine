#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/core/utils.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include "lib/core/logger.h"
#include <iostream>
#include <fstream>

std::unique_ptr<ImageWriter> ImageWriter::create(Config& config)
{
    if (!config.hasImageWriter())
        error("no image writer configured");
    return create(config->image_writer());
}

std::unique_ptr<ImageWriter> ImageWriter::create(const ImageWriterProto& config)
{
    switch (config.type())
    {
        case IMAGETYPE_IMG:
            return ImageWriter::createImgImageWriter(config);

        case IMAGETYPE_D64:
            return ImageWriter::createD64ImageWriter(config);

        case IMAGETYPE_LDBS:
            return ImageWriter::createLDBSImageWriter(config);

        case IMAGETYPE_DISKCOPY:
            return ImageWriter::createDiskCopyImageWriter(config);

        case IMAGETYPE_NSI:
            return ImageWriter::createNsiImageWriter(config);

        case IMAGETYPE_RAW:
            return ImageWriter::createRawImageWriter(config);

        case IMAGETYPE_D88:
            return ImageWriter::createD88ImageWriter(config);

        case IMAGETYPE_IMD:
            return ImageWriter::createImdImageWriter(config);

        default:
            error("bad output image config");
            return std::unique_ptr<ImageWriter>();
    }
}

ImageWriter::ImageWriter(const ImageWriterProto& config): _config(config) {}

void ImageWriter::writeCsv(const Image& image, const std::string& filename)
{
    std::ofstream f(filename, std::ios::out);
    if (!f.is_open())
        error("cannot open CSV report file");

    f << "\"Physical track\","
         "\"Physical side\","
         "\"Logical sector\","
         "\"Logical track\","
         "\"Logical side\","
         "\"Clock (ns)\","
         "\"Header start (ns)\","
         "\"Header end (ns)\","
         "\"Data start (ns)\","
         "\"Data end (ns)\","
         "\"Raw data address (bytes)\","
         "\"User payload length (bytes)\","
         "\"Status\""
         "\n";

    for (const auto& sector : image)
    {
        f << fmt::format("{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
            sector->physicalCylinder,
            sector->physicalHead,
            sector->logicalSector,
            sector->logicalCylinder,
            sector->logicalHead,
            sector->clock,
            sector->headerStartTime,
            sector->headerEndTime,
            sector->dataStartTime,
            sector->dataEndTime,
            sector->position,
            sector->data.size(),
            Sector::statusToString(sector->status));
    }
}

void ImageWriter::printMap(const Image& image)
{
    Geometry geometry = image.getGeometry();

    int badSectors = 0;
    int missingSectors = 0;
    int totalSectors = 0;

    std::cout << "     Tracks -> ";
    for (unsigned i = 10; i < geometry.numCylinders; i += 10)
        std::cout << fmt::format("{:<10d}", i / 10);
    std::cout << std::endl;
    std::cout << "H.SS ";
    for (unsigned i = 0; i < geometry.numCylinders; i++)
        std::cout << std::to_string(i % 10);
    std::cout << std::endl;

    for (int side = 0; side < geometry.numHeads; side++)
    {
        int maxSector = geometry.firstSector + geometry.numSectors - 1;
        for (int sectorId = 0; sectorId <= maxSector; sectorId++)
        {
            if (sectorId < geometry.firstSector)
                continue;

            std::cout << fmt::format("{}.{:2} ", side, sectorId);
            for (int track = 0; track < geometry.numCylinders; track++)
            {
                const auto& sector = image.get(track, side, sectorId);
                if (!sector)
                {
                    std::cout << 'X';
                    missingSectors++;
                }
                else
                {
                    switch (sector->status)
                    {
                        case Sector::OK:
                            std::cout << '.';
                            break;

                        case Sector::BAD_CHECKSUM:
                            std::cout << 'B';
                            badSectors++;
                            break;

                        case Sector::CONFLICT:
                            std::cout << 'C';
                            badSectors++;
                            break;

                        default:
                            std::cout << (int)sector->status;
                            break;
                    }
                }
                totalSectors++;
            }
            std::cout << std::endl;
        }
    }
    int goodSectors = totalSectors - missingSectors - badSectors;
    if (totalSectors == 0)
        std::cout << "No sectors in output; skipping analysis" << std::endl;
    else
    {
        std::cout << "Good sectors: " << goodSectors << "/" << totalSectors
                  << " (" << (100 * goodSectors / totalSectors) << "%)"
                  << std::endl;
        std::cout << "Missing sectors: " << missingSectors << "/"
                  << totalSectors << " ("
                  << (100 * missingSectors / totalSectors) << "%)" << std::endl;
        std::cout << "Bad sectors: " << badSectors << "/" << totalSectors
                  << " (" << (100 * badSectors / totalSectors) << "%)"
                  << std::endl;
    }
}
