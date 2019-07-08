#include "globals.h"
#include "fmt/format.h"
#include <fstream>
#include "fnmatch.h"

/* Theoretical maximum number of sectors. */
static const int SECTOR_COUNT = 640;

struct Dirent
{
    std::string filename;
    int type;
    int startSector;
    int sectorCount;
};

static std::ifstream inputFile;
static std::map<std::string, std::unique_ptr<Dirent>> directory;
static std::map<uint16_t, uint16_t> allocationTable;

static std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

void syntax()
{
    std::cout << "Syntax: brother120tool <image> [<filenames...>]\n"
                 "If you specify a filename, it's extracted into the current directory.\n"
                 "Wildcards are allowed. If you don't, the directory is listed instead.\n";
    exit(0);
}

void readDirectory()
{
    for (int i=0; i<0x80; i++)
    {
        inputFile.seekg(i*16, std::ifstream::beg);

        uint8_t buffer[16];
        inputFile.read((char*) buffer, sizeof(buffer));
        if (buffer[0] == 0xf0)
            continue;
        
        std::string filename((const char*)buffer, 8);
        filename = rtrim(filename);
        std::unique_ptr<Dirent> dirent(new Dirent);
        dirent->filename = filename;
        dirent->type = buffer[8];
        dirent->startSector = buffer[9]*256 + buffer[10];
        dirent->sectorCount = buffer[11];
        directory[filename] = std::move(dirent);
    }
}

static bool isValidFile(const Dirent& dirent)
{
    return (dirent.filename[0] & 0x80) == 0;
}

void readAllocationTable()
{
    for (int sector=14; sector!=SECTOR_COUNT; sector++)
    {
        inputFile.seekg((sector-1)*2 + 0x800, std::ifstream::beg);
        uint8_t buffer[2];
        inputFile.read((char*) buffer, sizeof(buffer));

        uint16_t nextSector = (buffer[0]*256) + buffer[1];
        allocationTable[sector] = nextSector;
    }
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

void extractFile(const std::string& pattern)
{
    for (const auto& i : directory)
    {
        const Dirent& dirent = *i.second;
        if (dirent.type != 0)
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
            inputFile.seekg((sector-1) * 0x100, std::ifstream::beg);
            if (!inputFile.read((char*) buffer, sizeof(buffer)))
                Error() << fmt::format("I/O error on read: {}", strerror(errno));
            if (!outputFile.write((const char*) buffer, sizeof(buffer)))
                Error() << fmt::format("I/O error on write: {}", strerror(errno));

            sector = allocationTable[sector];
        }
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
        syntax();
    
    inputFile.open(argv[1], std::ios::in | std::ios::binary);
    if (!inputFile.is_open())
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

    inputFile.close();
    return 0;
}
