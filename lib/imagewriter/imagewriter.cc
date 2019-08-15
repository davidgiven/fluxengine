#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<ImageWriter> ImageWriter::create(const SectorSet& sectors, const ImageSpec& spec)
{
    const auto& filename = spec.filename;

    if (ends_with(filename, ".img") || ends_with(filename, ".adf"))
        return createImgImageWriter(sectors, spec);
	else if (ends_with(filename, ".ldbs"))
		return createLDBSImageWriter(sectors, spec);
	else if (ends_with(filename, ".d64"))
		return createD64ImageWriter(sectors, spec);

    Error() << "unrecognised image filename extension";
    return std::unique_ptr<ImageWriter>();
}

ImageWriter::ImageWriter(const SectorSet& sectors, const ImageSpec& spec):
    sectors(sectors),
    spec(spec)
{}

void ImageWriter::adjustGeometry()
{
	if (!spec.initialised)
	{
		sectors.calculateSize(spec.cylinders, spec.heads, spec.sectors, spec.bytes);
        spec.initialised = true;
		std::cout << "Autodetecting output geometry\n";
	}
}

void ImageWriter::printMap()
{
	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;
	std::cout << "H.SS Tracks --->" << std::endl;
	for (int head = 0; head < spec.heads; head++)
	{
		for (int sectorId = 0; sectorId < spec.sectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < spec.cylinders; track++)
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
