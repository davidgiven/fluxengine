#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "image.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "lib/layout.h"
#include "lib/logger.h"
#include "fmt/format.h"
#include <iostream>
#include <fstream>

std::unique_ptr<ImageWriter> ImageWriter::create(const ImageWriterProto& config)
{
    switch (config.format_case())
    {
        case ImageWriterProto::kImg:
            return ImageWriter::createImgImageWriter(config);

        case ImageWriterProto::kD64:
            return ImageWriter::createD64ImageWriter(config);

        case ImageWriterProto::kLdbs:
            return ImageWriter::createLDBSImageWriter(config);

        case ImageWriterProto::kDiskcopy:
            return ImageWriter::createDiskCopyImageWriter(config);

        case ImageWriterProto::kNsi:
            return ImageWriter::createNsiImageWriter(config);

        case ImageWriterProto::kRaw:
            return ImageWriter::createRawImageWriter(config);

        case ImageWriterProto::kD88:
            return ImageWriter::createD88ImageWriter(config);

        case ImageWriterProto::kImd:
            return ImageWriter::createImdImageWriter(config);

        default:
            Error() << "bad output image config";
            return std::unique_ptr<ImageWriter>();
    }
}

void ImageWriter::updateConfigForFilename(
    ImageWriterProto* proto, const std::string& filename)
{
    static const std::map<std::string, std::function<void(ImageWriterProto*)>>
        formats = {
  // clang-format off
		{".adf",      [](auto* proto) { proto->mutable_img(); }},
		{".d64",      [](auto* proto) { proto->mutable_d64(); }},
		{".d81",      [](auto* proto) { proto->mutable_img(); }},
		{".d88",      [](auto* proto) { proto->mutable_d88(); }},
		{".diskcopy", [](auto* proto) { proto->mutable_diskcopy(); }},
		{".dsk",      [](auto* proto) { proto->mutable_img(); }},
		{".img",      [](auto* proto) { proto->mutable_img(); }},
		{".imd",      [](auto* proto) { proto->mutable_imd(); }},
		{".ldbs",     [](auto* proto) { proto->mutable_ldbs(); }},
		{".nsi",      [](auto* proto) { proto->mutable_nsi(); }},
		{".raw",      [](auto* proto) { proto->mutable_raw(); }},
		{".st",       [](auto* proto) { proto->mutable_img(); }},
		{".vgi",      [](auto* proto) { proto->mutable_img(); }},
		{".xdf",      [](auto* proto) { proto->mutable_img(); }},
  // clang-format on
    };

    for (const auto& it : formats)
    {
        if (endsWith(filename, it.first))
        {
            it.second(proto);
            proto->set_filename(filename);
            return;
        }
    }

    Error() << fmt::format("unrecognised image filename '{}'", filename);
}

ImageWriter::ImageWriter(const ImageWriterProto& config): _config(config) {}

void ImageWriter::writeCsv(const Image& image, const std::string& filename)
{
    std::ofstream f(filename, std::ios::out);
    if (!f.is_open())
        Error() << "cannot open CSV report file";

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
            sector->physicalTrack,
            sector->physicalHead,
            sector->logicalSector,
            sector->logicalTrack,
            sector->logicalSide,
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
    for (unsigned i = 10; i < geometry.numTracks; i += 10)
        std::cout << fmt::format("{:<10d}", i / 10);
    std::cout << std::endl;
    std::cout << "H.SS ";
    for (unsigned i = 0; i < geometry.numTracks; i++)
        std::cout << fmt::format("{}", i % 10);
    std::cout << std::endl;

    for (int side = 0; side < geometry.numSides; side++)
    {
        int maxSector = geometry.firstSector + geometry.numSectors - 1;
        for (int sectorId = 0; sectorId <= maxSector; sectorId++)
        {
            if (sectorId < geometry.firstSector)
                continue;

            std::cout << fmt::format("{}.{:2} ", side, sectorId);
            for (int track = 0; track < geometry.numTracks; track++)
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
                            std::cout << '?';
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

void ImageWriter::writeMappedImage(const Image& image)
{
    if (_config.filesystem_sector_order())
    {
        Logger()
            << "WRITER: converting from disk sector order to filesystem order";

        std::set<std::shared_ptr<const Sector>> sectors;
        for (const auto& e : image)
        {
            auto& trackLayout =
                Layout::getLayoutOfTrack(e->logicalTrack, e->logicalSide);
            auto newSector = std::make_shared<Sector>();
            *newSector = *e;
            newSector->logicalSector =
                trackLayout.logicalToFilesystemSectorMap.at(e->logicalSector);
            sectors.insert(newSector);
        }

        writeImage(Image(sectors));
    }
    else
        writeImage(image);
}
