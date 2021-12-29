#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "image.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "proto.h"
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

		default:
			Error() << "bad output image config";
			return std::unique_ptr<ImageWriter>();
	}
}

void ImageWriter::updateConfigForFilename(ImageWriterProto* proto, const std::string& filename)
{
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".adf",      [&]() { proto->mutable_img(); }},
		{".d64",      [&]() { proto->mutable_d64(); }},
		{".d81",      [&]() { proto->mutable_img(); }},
		{".diskcopy", [&]() { proto->mutable_diskcopy(); }},
		{".img",      [&]() { proto->mutable_img(); }},
		{".ldbs",     [&]() { proto->mutable_ldbs(); }},
		{".st",       [&]() { proto->mutable_img(); }},
		{".nsi",      [&]() { proto->mutable_nsi(); }},
		{".vgi",      [&]() { proto->mutable_img(); }},
		{".xdf",      [&]() { proto->mutable_img(); }},
	};

	for (const auto& it : formats)
	{
		if (endsWith(filename, it.first))
		{
			it.second();
			proto->set_filename(filename);
			return;
		}
	}

	Error() << fmt::format("unrecognised image filename '{}'", filename);
}

ImageWriter::ImageWriter(const ImageWriterProto& config):
	_config(config)
{}

void ImageWriter::writeCsv(const Image& image, const std::string& filename)
{
	std::ofstream f(filename, std::ios::out);
	if (!f.is_open())
		Error() << "cannot open CSV report file";

	f << "\"Physical track\","
		"\"Physical side\","
		"\"Logical track\","
		"\"Logical side\","
		"\"Logical sector\","
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
			sector->logicalTrack,
			sector->logicalSide,
			sector->logicalSector,
			sector->clock,
			sector->headerStartTime,
			sector->headerEndTime,
			sector->dataStartTime,
			sector->dataEndTime,
			sector->position.bytes,
			sector->data.size(),
			Sector::statusToString(sector->status)
		);
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
		std::cout << fmt::format("{:<10d}", i/10);
	std::cout << std::endl;
	std::cout << "H.SS ";
	for (unsigned i = 0; i < geometry.numTracks; i++)
		std::cout << fmt::format("{}", i%10);
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
				  << " (" << (100*goodSectors/totalSectors) << "%)"
				  << std::endl;
		std::cout << "Missing sectors: " << missingSectors << "/" << totalSectors
				  << " (" << (100*missingSectors/totalSectors) << "%)"
				  << std::endl;
		std::cout << "Bad sectors: " << badSectors << "/" << totalSectors
				  << " (" << (100*badSectors/totalSectors) << "%)"
				  << std::endl;
    }
}
