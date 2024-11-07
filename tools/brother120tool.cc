#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include "lib/core/utils.h"
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
                 "If you specify a filename, it's extracted into the current "
                 "directory.\n"
                 "Wildcards are allowed. If you don't, the directory is listed "
                 "instead.\n";
    exit(0);
}

void readDirectory()
{
    for (int i = 0; i < DIRECTORY_SIZE; i++)
    {
        file.seekg(i * 16, std::ifstream::beg);

        Bytes buffer(16);
        file.read((char*)&buffer[0], buffer.size());
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
            error("too many files on disk");

        bw.append(dirent->filename);
        for (int i = dirent->filename.size(); i < 8; i++)
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

void writeBootSector(const Dirent& dirent, uint16_t checksum)
{
    uint8_t sslo = dirent.startSector & 0xff;
    uint8_t sshi = dirent.startSector >> 8;
    uint8_t scnt = dirent.sectorCount;
    uint8_t end = 0x70 + scnt;
    uint8_t cklo = checksum & 0xff;
    uint8_t ckhi = checksum >> 8;

    uint8_t machineCode[] = {
        // clang-format off
		/* 6000 */ 0x55,             /* magic number?         */
		/* 6001 */ 0xf3,             /* di                    */
		/* 6002 */ 0x31, 0x00, 0x00, /* ld sp, $0000          */
		/* 6005 */ 0x3e, 0x00,       /* ld a, $00             */
		/* 6007 */ 0xed, 0x39, 0x34, /* out0 ($34), a         */
		/* 600a */ 0x3e, 0x0f,       /* ld a, $0f             */
		/* 600c */ 0xed, 0x39, 0x0a, /* out0 ($0a), a         */
		/* 600f */ 0x3e, 0x00,       /* ld a,$00              */
		/* 6011 */ 0xed, 0x39, 0xd8, /* out0 ($d8),a          */
		/* 6014 */ 0x3e, 0xe7,       /* ld a,$e7              */
		/* 6016 */ 0xed, 0x39, 0x3a, /* out0 ($3a),a          */
		/* 6019 */ 0x3e, 0x16,       /* ld a,$16              */
		/* 601b */ 0xed, 0x39, 0x38, /* out0 ($38),a          */
		/* 601e */ 0x3e, 0x20,       /* ld a,$20              */
		/* 6020 */ 0xed, 0x39, 0x39, /* out0 ($39),a          */
		/* 6023 */ 0x01, sslo, sshi, /* ld bc, start sector   */
		/* 6026 */ 0x21, 0x00, 0xf8, /* ld hl,$f800           */
		/* 6029 */ 0x1e, scnt,       /* ld e, sector count    */
		/* 602b */ 0x16, 0x70,       /* ld d,$70              */
		/* 602d */ 0xc5,             /* push bc               */
		/* 602e */ 0xd5,             /* push de               */
		/* 602f */ 0xe5,             /* push hl               */
		/* 6030 */ 0x3e, 0x06,       /* ld a,$06              */
		/* 6032 */ 0xef,             /* rst $28               */
		/* 6033 */ 0xda, 0xd3, 0x60, /* jp c,$60d3            */
		/* 6036 */ 0xe1,             /* pop hl                */
		/* 6037 */ 0xd1,             /* pop de                */
		/* 6038 */ 0xc1,             /* pop bc                */
		/* 6039 */ 0x3e, 0x00,       /* ld a,$00              */
		/* 603b */ 0xed, 0x39, 0x20, /* out0 ($20),a          */
		/* 603e */ 0x3e, 0x58,       /* ld a,$58              */
		/* 6040 */ 0xed, 0x39, 0x21, /* out0 ($21),a          */
		/* 6043 */ 0x3e, 0x02,       /* ld a,$02              */
		/* 6045 */ 0xed, 0x39, 0x22, /* out0 ($22),a          */
		/* 6048 */ 0x3e, 0x00,       /* ld a,$00              */
		/* 604a */ 0xed, 0x39, 0x23, /* out0 ($23),a          */
		/* 604d */ 0x7a,             /* ld a,d                */
		/* 604e */ 0xed, 0x39, 0x24, /* out0 ($24),a          */
		/* 6051 */ 0x3e, 0x02,       /* ld a,$02              */
		/* 6053 */ 0xed, 0x39, 0x25, /* out0 ($25),a          */
		/* 6056 */ 0x3e, 0x00,       /* ld a,$00              */
		/* 6058 */ 0xed, 0x39, 0x26, /* out0 ($26),a          */
		/* 605b */ 0x3e, 0x01,       /* ld a,$01              */
		/* 605d */ 0xed, 0x39, 0x27, /* out0 ($27),a          */
		/* 6060 */ 0x3e, 0x02,       /* ld a,$02              */
		/* 6062 */ 0xed, 0x39, 0x31, /* out0 ($31),a          */
		/* 6065 */ 0x3e, 0x40,       /* ld a,$40              */
		/* 6067 */ 0xed, 0x39, 0x30, /* out0 ($30),a          */
		/* 606a */ 0x03,             /* inc bc                */
		/* 606b */ 0x14,             /* inc d                 */
		/* 606c */ 0x1d,             /* dec e                 */
		/* 606d */ 0x7b,             /* ld a,e                */
		/* 606e */ 0xfe, 0x00,       /* cp $00                */
		/* 6070 */ 0x20, 0xbb,       /* jr nz,$602d           */
		/* 6072 */ 0x3e, 0x02,       /* ld a,$02              */
		/* 6074 */ 0xef,             /* rst $28               */
		/* 6075 */ 0x3e, 0x20,       /* ld a,$20              */
		/* 6077 */ 0xed, 0x39, 0x38, /* out0 ($38),a          */
		/* 607a */ 0x3e, 0x0c,       /* ld a,$0c              */
		/* 607c */ 0xed, 0x39, 0x39, /* out0 ($39),a          */
		/* 607f */ 0x3e, 0x64,       /* ld a,$64              */
		/* 6081 */ 0xed, 0x39, 0x3a, /* out0 ($3a),a          */
		/* 6084 */ 0x3e, 0x0f,       /* ld a,$0f              */
		/* 6086 */ 0xed, 0x39, 0x0a, /* out0 ($0a),a          */

		/* checksum routine */
		/* 6089 */ 0x01, 0x00, 0x70, /* ld bc,$7000           */
		/* 608c */ 0x11, 0x00, 0x00, /* ld de,$0000           */
		/* 608f */ 0x21, 0x00, 0x00, /* ld hl,$0000           */
		/* 6092 */ 0x0a,             /* ld a,(bc)             */
		/* 6093 */ 0x5f,             /* ld e,a                */
		/* 6094 */ 0x19,             /* add hl,de             */
		/* 6095 */ 0x03,             /* inc bc                */
		/* 6096 */ 0x79,             /* ld a,c                */
		/* 6097 */ 0xfe, 0x00,       /* cp $00                */
		/* 6099 */ 0x20, 0xf7,       /* jr nz,$6092           */
		/* 609b */ 0x78,             /* ld a,b                */
		/* 609c */ 0xfe, end,        /* cp end page           */
		/* 609e */ 0x20, 0xf2,       /* jr nz,$6092           */
		/* 60a0 */ 0x11, 0xf3, 0x60, /* ld de,$60f3           */
		/* 60a3 */ 0x1a,             /* ld a,(de)             */
		/* 60a4 */ 0xbd,             /* cp l                  */
		/* 60a5 */ 0x20, 0x2f,       /* jr nz,$60d6           */
		/* 60a7 */ 0x13,             /* inc de                */
		/* 60a8 */ 0x1a,             /* ld a,(de)             */
		/* 60a9 */ 0xbc,             /* cp h                  */
		/* 60aa */ 0x20, 0x2a,       /* jr nz,$60d6           */

		/* reset and execute */
		/* 60ac */ 0x3e, 0xff,       /* ld a,$ff              */
		/* 60ae */ 0xed, 0x39, 0x88, /* out0 ($88),a          */
		/* 60b1 */ 0x01, 0xff, 0x0f, /* ld bc,$0fff           */
		/* 60b4 */ 0x21, 0x00, 0x40, /* ld hl,$4000           */
		/* 60b7 */ 0x11, 0x01, 0x40, /* ld de,$4001           */
		/* 60ba */ 0x36, 0x00,       /* ld (hl),$00           */
		/* 60bc */ 0xed, 0xb0,       /* ldir                  */
		/* 60be */ 0x01, 0xff, 0x0f, /* ld bc,$0fff           */
		/* 60c1 */ 0x21, 0x00, 0x50, /* ld hl,$5000           */
		/* 60c4 */ 0x11, 0x01, 0x50, /* ld de,$5001           */
		/* 60c7 */ 0x36, 0x20,       /* ld (hl),$20           */
		/* 60c9 */ 0xed, 0xb0,       /* ldir                  */
		/* 60cb */ 0x3e, 0xfe,       /* ld a,$fe              */
		/* 60cd */ 0xed, 0x39, 0x88, /* out0 ($88),a          */
		/* 60d0 */ 0xc3, 0x00, 0x70, /* jp $7000              */

		/* 60d3 */ 0xe1,             /* pop hl                */
		/* 60d4 */ 0xd1,             /* pop de                */
		/* 60d5 */ 0xc1,             /* pop bc                */
		/* 60d6 */ 0x01, 0x00, 0x00, /* ld bc,$0000           */
		/* 60d9 */ 0x0b,             /* dec bc                */
		/* 60da */ 0x3e, 0xfe,       /* ld a,$fe              */
		/* 60dc */ 0xed, 0x39, 0x90, /* out0 ($90),a          */
		/* 60df */ 0x78,             /* ld a,b                */
		/* 60e0 */ 0xb1,             /* or c                  */
		/* 60e1 */ 0x20, 0xf6,       /* jr nz,$60d9           */
		/* 60e3 */ 0x31, 0x00, 0x00, /* ld sp,$0000           */
		/* 60e6 */ 0x3e, 0xff,       /* ld a,$ff              */
		/* 60e8 */ 0xed, 0x39, 0x90, /* out0 ($90),a          */
		/* 60eb */ 0x3e, 0x0f,       /* ld a,$0f              */
		/* 60ed */ 0xed, 0x39, 0x0a, /* out0 ($0a),a          */
		/* 60f0 */ 0xc3, 0x00, 0x00, /* jp $0000              */
		/* 60f3 */ cklo, ckhi,       /* checksum              */
        // clang-format on
    };

    file.seekp(0xc00, std::ifstream::beg);
    file.write((char*)machineCode, sizeof(machineCode));
}

static bool isValidFile(const Dirent& dirent)
{
    return (dirent.filename[0] & 0x80) == 0;
}

void readAllocationTable()
{
    for (int sector = 1; sector != SECTOR_COUNT; sector++)
    {
        file.seekg((sector - 1) * 2 + 0x800, std::ifstream::beg);
        Bytes buffer(2);
        file.read((char*)&buffer[0], buffer.size());

        uint16_t nextSector = buffer.reader().read_be16();
        allocationTable[sector] = nextSector;
    }
}

void writeAllocationTable()
{
    Bytes buffer(SECTOR_COUNT * 2);
    ByteWriter bw(buffer);

    for (int sector = 1; sector < (DATA_START_SECTOR - 2); sector++)
        bw.write_le16(sector + 1);
    bw.write_le16(0xffff);
    bw.write_le16(0xffff);
    for (int sector = DATA_START_SECTOR; sector != SECTOR_COUNT; sector++)
        bw.write_be16(allocationTable[sector]);

    file.seekp(0x800, std::ifstream::beg);
    buffer.writeTo(file);
}

uint16_t allocateSector()
{
    for (int sector = DATA_START_SECTOR; sector != SECTOR_COUNT; sector++)
    {
        if (allocationTable[sector] == 0)
        {
            allocationTable[sector] = 0xffff;
            return sector;
        }
    }
    error("unable to allocate sector --- disk full");
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
                std::cout << fmt::format(
                    "warning: sector {} appears to be multiply used\n", sector);
            bitmap[sector] = true;
            sector = allocationTable[sector];
            count++;
        }

        if (count != dirent.sectorCount)
            std::cout << fmt::format(
                "Warning: file '{}' claims to be {} sectors long but its chain "
                "is {}\n",
                dirent.filename,
                dirent.sectorCount,
                count);
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
                dirent.sectorCount,
                dirent.startSector);
        else
            std::cout << "DELETED";

        std::cout << std::endl;
    }
}

void insertFile(const std::string& filename)
{
    auto leafname = getLeafname(filename);
    if (leafname.size() > 8)
        error("filename too long");
    std::cout << fmt::format("Inserting '{}'\n", leafname);

    std::ifstream inputFile(filename, std::ios::in | std::ios::binary);
    if (!inputFile)
        error("unable to open input file: {}", strerror(errno));

    if (directory.find(leafname) != directory.end())
        error("duplicate filename: {}", leafname);

    auto dirent = std::make_unique<Dirent>();
    dirent->filename = leafname;
    dirent->type = (leafname.find('*') != std::string::npos);
    dirent->startSector = 0xffff;
    dirent->sectorCount = 0;

    uint16_t lastSector = 0xffff;
    uint16_t checksum = 0;
    while (!inputFile.eof())
    {
        uint8_t buffer[SECTOR_SIZE] = {};
        inputFile.read((char*)buffer, sizeof(buffer));
        for (int i = 0; i < inputFile.gcount(); i++)
            checksum += buffer[i];
        if (inputFile.gcount() == 0)
            break;
        if (inputFile.bad())
            error("I/O error on read: {}", strerror(errno));

        uint16_t thisSector = allocateSector();
        if (lastSector == 0xffff)
            dirent->startSector = thisSector;
        else
            allocationTable[lastSector] = thisSector;
        dirent->sectorCount++;

        file.seekp((thisSector - 1) * 0x100, std::ifstream::beg);
        file.write((char*)buffer, sizeof(buffer));
        if (file.bad())
            error("I/O error on write: {}", strerror(errno));

        lastSector = thisSector;
    }

    if (leafname == "*boot")
    {
        std::cout << fmt::format(
            "Writing boot sector with checksum 0x{:04x}\n", checksum);
        writeBootSector(*dirent, checksum);
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
            error("unable to open output file: {}", strerror(errno));

        uint16_t sector = dirent.startSector;
        while ((sector != 0) && (sector != 0xffff))
        {
            uint8_t buffer[256];
            file.seekg((sector - 1) * 0x100, std::ifstream::beg);
            if (!file.read((char*)buffer, sizeof(buffer)))
                error("I/O error on read: {}", strerror(errno));
            if (!outputFile.write((const char*)buffer, sizeof(buffer)))
                error("I/O error on write: {}", strerror(errno));

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
        error("cannot open output file '{}'", argv[1]);

    file.seekp(SECTOR_COUNT * SECTOR_SIZE - 1, std::ifstream::beg);
    file.put(0);

    for (int i = 2; i < argc; i++)
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
        error("cannot open input file '{}'", argv[1]);

    readDirectory();
    readAllocationTable();
    checkConsistency();

    if (argc == 2)
        listDirectory();
    else
    {
        for (int i = 2; i < argc; i++)
            extractFile(argv[i]);
    }

    file.close();
}

int main(int argc, const char* argv[])
{
    try
    {
        if ((argc > 1) && (strcmp(argv[1], "--create") == 0))
            doCreate(argc - 1, argv + 1);
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
