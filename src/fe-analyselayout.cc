#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxmap.h"
#include "sector.h"
#include "sectorset.h"
#include "csvreader.h"
#include "visualiser.h"
#include "decoders/fluxmapreader.h"
#include "dep/agg/include/agg2d.h"
#include "dep/stb/stb_image_write.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags = {
	&visualiserFlags
};

static StringFlag source(
    { "--csv", "-s" },
    "CSV file produced by reader",
    "");

static DataSpecFlag writeImg(
	{ "--write-img" },
	"Draw a graph of the disk layout",
	":w=640:h=480");

static std::ifstream inputFile;

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

