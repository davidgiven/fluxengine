#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <iostream>
#include <fstream>

#if 0
std::map<std::string, ImageWriter::Constructor> ImageWriter::formats =
{
	{".adf", ImageWriter::createImgImageWriter},
	{".d64", ImageWriter::createD64ImageWriter},
	{".d81", ImageWriter::createImgImageWriter},
	{".diskcopy", ImageWriter::createDiskCopyImageWriter},
	{".img", ImageWriter::createImgImageWriter},
	{".ldbs", ImageWriter::createLDBSImageWriter},
	{".st", ImageWriter::createImgImageWriter},
};
#endif

std::unique_ptr<ImageWriter> ImageWriter::create(const OutputFileProto& config)
{
	if (config.has_img())
		return ImageWriter::createImgImageWriter(config);
	else
		Error() << "bad output image config";
}

//void ImageWriter::verifyImageSpec(const ImageSpec& spec)
//{
//	if (!findConstructor(spec))
//		Error() << "unrecognised output image filename extension";
//}

ImageWriter::ImageWriter(const OutputFileProto& config):
	_config(config)
{}

void ImageWriter::writeCsv(const SectorSet& sectors, const std::string& filename)
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

	for (const auto& it : sectors.get())
	{
		const auto& sector = it.second;
		f << fmt::format("{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
			sector->physicalTrack,
			sector->physicalSide,
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

void ImageWriter::printMap(const SectorSet& sectors)
{
	unsigned numCylinders;
	unsigned numHeads;
	unsigned numSectors;
	unsigned numBytes;
	sectors.calculateSize(numCylinders, numHeads, numSectors, numBytes);

	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;
	std::cout << "H.SS Tracks --->" << std::endl;
	for (int head = 0; head < numHeads; head++)
	{
		for (int sectorId = 0; sectorId < numSectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < numCylinders; track++)
			{
				const auto& sector = sectors.get(track, head, sectorId);
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
