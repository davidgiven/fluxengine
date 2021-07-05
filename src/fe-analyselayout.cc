#define _USE_MATH_DEFINES
#include "globals.h"
#include "flags.h"
#include "bitmap.h"
#include "fluxmap.h"
#include "sector.h"
#include "sectorset.h"
#include "csvreader.h"
#include "decoders/fluxmapreader.h"
#include "dep/agg/include/agg2d.h"
#include "dep/stb/stb_image_write.h"
#include "fmt/format.h"
#include <math.h>
#include <fstream>

static FlagGroup flags = {};

static StringFlag source(
    { "--csv", "-s" },
    "CSV file produced by reader",
    "");

static StringFlag writeImg(
	{ "--img", "-o" },
	"Draw a graph of the disk layout",
	"disklayout.png");

static IntFlag imgWidth(
	{ "--width" },
	"Width of output graph",
	800);

static IntFlag imgHeight(
	{ "--height" },
	"Height of output graph",
	600);

static IntFlag period(
    { "--visualiser-period" },
    "rotational period for use by the visualiser (milliseconds)",
    200);

static IntFlag sideToDraw(
	{ "--side" },
	"side to draw; -1 for both",
	-1);

static IntFlag alignWithSector(
	{ "--align" },
	"sector to align to; -1 to align to index mark",
	-1);

static std::ifstream inputFile;

static const int BORDER = 10;
static const int TRACKS = 83;

void visualiseSectorsToFile(const SectorSet& sectors, const std::string& filename)
{
	Bitmap bitmap(writeImg, imgWidth, imgHeight);
	if (bitmap.filename.empty())
		Error() << "you must specify an image filename to write to";

	Agg2D& painter = bitmap.painter();
	painter.clearAll(0xff, 0xff, 0xff);

    const double radians_per_ns = 2.0*M_PI / (period*1e6);
	const double available_width = bitmap.width - 2*BORDER;
	const double available_height = bitmap.height - 2*BORDER;
	const double panel_centre = (sideToDraw == -1)
			? (available_width / 4)
			: (available_width / 2);
	const double panel_size = (sideToDraw == -1)
			? std::min(available_width / 2, available_height)
			: std::min(available_width, available_height);
	const double disk_radius = (panel_size-BORDER) / 2;

    auto drawSide = [&](int side)
    {
		int xpos = BORDER +
			((sideToDraw == -1)
				? (panel_centre + side*panel_size)
				: panel_centre);

        for (int physicalTrack = 0; physicalTrack < TRACKS; physicalTrack++)
        {
			double visibleDistance = (TRACKS * 0.5) + (TRACKS - physicalTrack);
			double radius = (disk_radius*visibleDistance)/(TRACKS * 1.5);
			painter.noFill();
			painter.lineColor(0x88, 0x88, 0x88);
			painter.lineWidth(disk_radius/(TRACKS*2));
			painter.ellipse(xpos, available_height/2, radius, radius);

			nanoseconds_t offset = 0;
			if (alignWithSector != -1)
			{
				for (const auto& e : sectors.get())
				{
					const auto& sector = e.second;
					if ((sector->physicalSide == side) && (sector->physicalTrack == physicalTrack)
							&& (sector->logicalSector == alignWithSector))
					{
						offset = sector->headerStartTime;
						if (!offset)
							offset = sector->dataStartTime;
						break;
					}
				}
			}

            auto drawArc = [&](nanoseconds_t start, nanoseconds_t end)
            {
                start = fmod(start, period*1000000.0);
                end = fmod(end, period*1000000.0);
                if (end < start)
                    end += period*1000000;
                
                double theta1 = (start - offset) * radians_per_ns;
                double theta2 = (end - offset) * radians_per_ns;
                int large = (theta2 - theta1) >= M_PI;

				painter.arc(xpos, available_height/2, radius, radius, theta1, theta2-theta1);
            };

            /* Sadly, SectorSets aren't indexable by physical track. */
            for (const auto& e : sectors.get())
            {
                const auto& sector = e.second;
                if ((sector->physicalSide == side) && (sector->physicalTrack == physicalTrack))
                {
					painter.lineColor(0xff, 0x00, 0x00);
                    if (sector->status == Sector::OK)
						painter.lineColor(0x00, 0x00, 0xff);
                    if (sector->dataStartTime && sector->dataEndTime)
                        drawArc(sector->dataStartTime, sector->dataEndTime);
                    if (sector->headerStartTime && sector->headerEndTime)
					{
						painter.lineColor(0x00, 0xff, 0xff);
                        drawArc(sector->headerStartTime, sector->headerEndTime);
					}
                }
            }

			painter.lineColor(0xff, 0xff, 0x00);
			drawArc(0, 2000);
        }
    };

	switch (sideToDraw)
	{
		case -1:
			drawSide(0);
			drawSide(1);
			break;

		case 0:
			drawSide(0);
			break;

		case 1:
			drawSide(1);
			break;
	}

	bitmap.save();
}

static void check_for_error()
{
    if (inputFile.fail())
        Error() << fmt::format("I/O error: {}", strerror(errno));
}

static void bad_csv()
{
	Error() << "bad CSV file format";
}

static void readRow(const std::vector<std::string>& row, SectorSet& sectors)
{
	if (row.size() != 13)
		bad_csv();

	try
	{
		Sector::Status status = Sector::stringToStatus(row[12]);
		if (status == Sector::Status::INTERNAL_ERROR)
			bad_csv();
		if (status == Sector::Status::MISSING)
			return;

		int physicalTrack = std::stoi(row[0]);
		int physicalSide = std::stoi(row[1]);
		int logicalSector = std::stoi(row[4]);

		Sector* sector = sectors.add(physicalTrack, physicalSide, logicalSector);
		sector->physicalTrack = physicalTrack;
		sector->physicalSide = physicalSide;
		sector->logicalTrack = std::stoi(row[2]);
		sector->logicalSide = std::stoi(row[3]);
		sector->logicalSector = logicalSector;
		sector->clock = std::stod(row[5]);
		sector->headerStartTime = std::stod(row[6]);
		sector->headerEndTime = std::stod(row[7]);
		sector->dataStartTime = std::stod(row[8]);
		sector->dataEndTime = std::stod(row[9]);
		sector->status = status;
	}
	catch (const std::invalid_argument& e)
	{
		bad_csv();
	}
}

static SectorSet readCsv(const std::string& filename)
{
	if (filename == "")
		Error() << "you must specify an input CSV file";

	inputFile.open(filename);
	check_for_error();

	CsvReader csvReader(inputFile);
	std::vector<std::string> row = csvReader.readLine();
	if (row.size() != 13)
		bad_csv();

	SectorSet sectors;
	for (;;)
	{
		row = csvReader.readLine();
		if (row.size() == 0)
			break;

		readRow(row, sectors);
	}

	return sectors;
}

int mainAnalyseLayout(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

	SectorSet sectors = readCsv(source.get());
	visualiseSectorsToFile(sectors, "out.svg");

	return 0;
}

