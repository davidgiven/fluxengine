#include "globals.h"
#include "bytes.h"
#include "fmt/format.h"
#include "utils.h"
#include <fstream>
#include "fnmatch.h"

/* Number of sectors on a 120kB disk. */
static constexpr int SECTOR_COUNT = 468;

/* Start sector for data (after the directory */
static constexpr int DATA_START_SECTOR = 14;

/* Size of a sector */
static constexpr int SECTOR_SIZE = 256;

/* Number of dirents in a directory. */
static constexpr int DIRECTORY_SIZE = 128;

struct Dirent
{
    std::string filename;
    int type;
    int startSector;
    int sectorCount;
};

static std::fstream file;
static std::map<std::string, std::unique_ptr<Dirent>> directory;
static std::map<uint16_t, uint16_t> allocationTable;

void syntax()
{
    std::cout << "Syntax: brother120tool <image> [<filenames...>]\n"
	             "        brother120tool --create <image> <filenames...>\n"
                 "If you specify a filename, it's extracted into the current directory.\n"
                 "Wildcards are allowed. If you don't, the directory is listed instead.\n";
    exit(0);
}

void readDirectory()
{
    for (int i=0; i<DIRECTORY_SIZE; i++)
    {
        file.seekg(i*16, std::ifstream::beg);

		Bytes buffer(16);
        file.read((char*) &buffer[0], buffer.size());
        if (buffer[0] == 0xf0)
            continue;
        
		ByteReader br(buffer);
		std::string filename = br.read(8);
		filename = filename.substr(0, filename.find(" "));

        std::unique_ptr<Dirent> dirent(new Dirent);
		dirent->filename = filename;
		dirent->type = br.read_8();
		dirent->startSector = br.read_be16();
		dirent->sectorCount = br.read_8();
        directory[filename] = std::move(dirent);
    }
}

void writeDirectory()
{
	Bytes buffer(2048);
	ByteWriter bw(buffer);

	int count = 0;
	for (const auto& it : directory)
	{
		const auto& dirent = it.second;

		if (count == DIRECTORY_SIZE)
			Error() << "too many files on disk";

		bw.append(dirent->filename);
		for (int i=dirent->filename.size(); i<8; i++)
			bw.write_8(' ');

		bw.write_8(dirent->type);
		bw.write_be16(dirent->startSector);
		bw.write_8(dirent->sectorCount);
		bw.write_be32(0); /* unknown */
		count++;
	}

	static const Bytes padding(15);
	while (count < DIRECTORY_SIZE)
	{
		bw.write_8(0xf0);
		bw.append(padding);
		count++;
	}

	file.seekp(0, std::ifstream::beg);
	buffer.writeTo(file);
}

static bool isValidFile(const Dirent& dirent)
{
    return (dirent.filename[0] & 0x80) == 0;
}

void readAllocationTable()
{
    for (int sector=1; sector!=SECTOR_COUNT; sector++)
    {
        file.seekg((sector-1)*2 + 0x800, std::ifstream::beg);
		Bytes buffer(2);
        file.read((char*) &buffer[0], buffer.size());

        uint16_t nextSector = buffer.reader().read_be16();
        allocationTable[sector] = nextSector;
    }
}

void writeAllocationTable()
{
	Bytes buffer(SECTOR_COUNT*2);
	ByteWriter bw(buffer);

	for (int sector=1; sector<(DATA_START_SECTOR-2); sector++)
		bw.write_le16(sector+1);
	bw.write_le16(0xffff);
	bw.write_le16(0xffff);
	for (int sector=DATA_START_SECTOR; sector!=SECTOR_COUNT; sector++)
		bw.write_be16(allocationTable[sector]);
	
	file.seekp(0x800, std::ifstream::beg);
	buffer.writeTo(file);
}

uint16_t allocateSector()
{
    for (int sector=DATA_START_SECTOR; sector!=SECTOR_COUNT; sector++)
    {
		if (allocationTable[sector] == 0)
		{
			allocationTable[sector] = 0xffff;
			return sector;
		}
    }
	Error() << "unable to allocate sector --- disk full";
	return 0;
}

void checkConsistency()
{
    /* Verify that we more-or-less understand the format by fscking the disk. */

    std::vector<bool> bitmap(640);
    for (const auto& i : directory)
    {
        const Dirent& dirent = *i.second;
        if (!isValidFile(dirent))
            continue;
        
        int count = 0;
        uint16_t sector = dirent.startSector;
        while ((sector != 0xffff) && (sector != 0))
        {
            if (bitmap[sector])
                std::cout << fmt::format("warning: sector {} appears to be multiply used\n", sector);
            bitmap[sector] = true;
            sector = allocationTable[sector];
            count++;
        }

        if (count != dirent.sectorCount)
            std::cout <<
                fmt::format("Warning: file '{}' claims to be {} sectors long but its chain is {}\n",
                    dirent.filename, dirent.sectorCount, count);
    }
}

void listDirectory()
{
    for (const auto& i : directory)
    {
        const Dirent& dirent = *i.second;
        std::cout << fmt::format("{:9} {:6.2f}kB type {}: ",
                        dirent.filename,
                        (double)dirent.sectorCount / 4.0,
                        dirent.type);

        if (isValidFile(dirent))
            std::cout << fmt::format("{} sectors starting at sector {}",
                dirent.sectorCount, dirent.startSector);
        else
                std::cout << "DELETED";

        std::cout << std::endl;
    }
}

void insertFile(const std::string& filename)
{
	auto leafname = getLeafname(filename);
	if (leafname.size() > 8)
		Error() << "filename too long";
	std::cout << fmt::format("Inserting '{}'\n", leafname);

	std::ifstream inputFile(filename, std::ios::in | std::ios::binary);
	if (!inputFile)
		Error() << fmt::format("unable to open input file: {}", strerror(errno));

	if (directory.find(leafname) != directory.end())
		Error() << fmt::format("duplicate filename: {}", leafname);

	auto dirent = std::make_unique<Dirent>();
	dirent->filename = leafname;
	dirent->type = (leafname.find('*') != std::string::npos);
	dirent->startSector = 0xffff;
	dirent->sectorCount = 0;

	uint16_t lastSector = 0xffff;
	while (!inputFile.eof())
	{
		char buffer[SECTOR_SIZE];
		inputFile.read(buffer, sizeof(buffer));
		if (inputFile.gcount() == 0)
			break;
		if (inputFile.bad())
			Error() << fmt::format("I/O error on read: {}", strerror(errno));

		uint16_t thisSector = allocateSector();
		if (lastSector == 0xffff)
			dirent->startSector = thisSector;
		else
			allocationTable[lastSector] = thisSector;
		dirent->sectorCount++;

		file.seekp((thisSector-1) * 0x100, std::ifstream::beg);
		file.write(buffer, sizeof(buffer));
		if (file.bad())
			Error() << fmt::format("I/O error on write: {}", strerror(errno));

		lastSector = thisSector;
	}

	directory[leafname] = std::move(dirent);
}

void extractFile(const std::string& pattern)
{
    for (const auto& i : directory)
    {
        const Dirent& dirent = *i.second;
        if ((dirent.type == 0xf0) || (dirent.type == 0xe5))
            continue;

        if (fnmatch(pattern.c_str(), dirent.filename.c_str(), 0))
            continue;

        std::cout << fmt::format("Extracting '{}'\n", dirent.filename);

        std::ofstream outputFile(dirent.filename,
            std::ios::out | std::ios::binary | std::ios::trunc);
        if (!outputFile)
            Error() << fmt::format("unable to open output file: {}", strerror(errno));

        uint16_t sector = dirent.startSector;
        while ((sector != 0) && (sector != 0xffff))
        {
            uint8_t buffer[256];
            file.seekg((sector-1) * 0x100, std::ifstream::beg);
            if (!file.read((char*) buffer, sizeof(buffer)))
                Error() << fmt::format("I/O error on read: {}", strerror(errno));
            if (!outputFile.write((const char*) buffer, sizeof(buffer)))
                Error() << fmt::format("I/O error on write: {}", strerror(errno));

            sector = allocationTable[sector];
        }
    }
}

static void doCreate(int argc, const char* argv[])
{
	if (argc < 3)
		syntax();

	file.open(argv[1], std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		Error() << fmt::format("cannot open output file '{}'", argv[1]);

	file.seekp(SECTOR_COUNT*SECTOR_SIZE - 1, std::ifstream::beg);
	file.put(0);

	for (int i=2; i<argc; i++)
		insertFile(argv[i]);
	
	writeDirectory();
	writeAllocationTable();
	checkConsistency();

	file.close();
}

static void doExtract(int argc, const char* argv[])
{
	if (argc < 2)
		syntax();
	
	file.open(argv[1], std::ios::in | std::ios::binary);
	if (!file.is_open())
		Error() << fmt::format("cannot open input file '{}'", argv[1]);

	readDirectory();
	readAllocationTable();
	checkConsistency();

	if (argc == 2)
		listDirectory();
	else
	{
		for (int i=2; i<argc; i++)
			extractFile(argv[i]);
	}

	file.close();
}

int main(int argc, const char* argv[])
{
	try
	{
		if ((argc > 1) && (strcmp(argv[1], "--create") == 0))
			doCreate(argc-1, argv+1);
		else
			doExtract(argc, argv);
	}
	catch (const ErrorException& e)
	{
		e.print();
		exit(1);
	}
    return 0;
}
