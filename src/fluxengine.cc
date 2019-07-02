#include "globals.h"

typedef int command_cb(int agrc, const char* argv[]);

extern command_cb mainErase;
extern command_cb mainInspect;
extern command_cb mainReadADFS;
extern command_cb mainReadAESLanier;
extern command_cb mainReadAmiga;
extern command_cb mainReadAmpro;
extern command_cb mainReadApple2;
extern command_cb mainReadBrother;
extern command_cb mainReadC64;
extern command_cb mainReadDFS;
extern command_cb mainReadF85;
extern command_cb mainReadFB100;
extern command_cb mainReadIBM;
extern command_cb mainReadVictor9K;
extern command_cb mainReadMac;
extern command_cb mainReadZilogMCZ;
extern command_cb mainRpm;
extern command_cb mainSeek;
extern command_cb mainTestBulkTransport;
extern command_cb mainUpgradeFluxFile;
extern command_cb mainWriteBrother;
extern command_cb mainWriteFlux;
extern command_cb mainWriteTestPattern;

struct Command
{
    std::string name;
    command_cb* main;
    std::string help;
};

static Command commands[] =
{
    {
        "erase", mainErase,
        "Permanently but rapidly erases some or all of a disk."
    },
    {
        "inspect", mainInspect,
        "Low-level analysis and inspection of a disk."
    },
    {
        "readadfs", mainReadADFS,
        "Reads Acorn ADFS disks.",
    },
    {
        "readaeslanier", mainReadAESLanier,
        "Reads AES Lanier disks.",
    },
    {
        "readamiga", mainReadAmiga,
        "Reads Commodore Amiga disks.",
    },
    {
        "readampro", mainReadAmpro,
        "Reads Ampro disks.",
    },
    {
        "readapple2", mainReadApple2,
        "Reads Apple II disks.",
    },
    {
        "readbrother", mainReadBrother,
        "Reads 120kB and 240kB Brother word processor disks.",
    },
    {
        "readc64", mainReadC64,
        "Reads Commodore 64 disks.",
    },
    {
        "readdfs", mainReadDFS,
        "Reads Acorn DFS disks.",
    },
    {
        "readf85", mainReadF85,
        "Reads Durango F85 disks.",
    },
    {
        "readfb100", mainReadFB100,
        "Reads FB100 disks.",
    },
    {
        "readibm", mainReadIBM,
        "Reads the ubiquitous IBM format disks.",
    },
    {
        "readmac", mainReadMac,
        "Reads Apple Macintosh disks.",
    },
    {
        "readvictor9k", mainReadVictor9K,
        "Reads Victor 9000 disks.",
    },
    {
        "readzilogmcz", mainReadZilogMCZ,
        "Reads Zilog MCZ disks.",
    },
    {
        "rpm", mainRpm,
        "Measures the disk rotational speed.",
    },
    {
        "seek", mainSeek,
        "Moves the disk head.",
    },
    {
        "testbulktransport", mainTestBulkTransport,
        "Measures your USB bandwidth.",
    },
    {
        "upgradefluxfile", mainUpgradeFluxFile,
        "Upgrades a flux file from a previous version of this software.",
    },
    {
        "writebrother", mainWriteBrother,
        "Writes 120kB and 240kB Brother word processor disks.",
    },
    {
        "writeflux", mainWriteFlux,
        "Writes a raw flux file. Warning: you can't use this to copy disks.",
    },
    {
        "writetestpattern", mainWriteTestPattern,
        "Writes a machine-generated test pattern to a disk.",
    },
};

static void help()
{
    std::cout << "fluxengine: syntax: fluxengine <command> [<flags>...]\n"
                 "Try one of these commands:\n";

    for (Command& c : commands)
        std::cout << "  " << c.name << ": " << c.help << "\n";

    exit(0);
}

int main(int argc, const char* argv[])
{
    if (argc == 1)
        help();

    std::string command = argv[1];
    if (command == "help")
        help();

    for (Command& c : commands)
    {
        if (command == c.name)
            return c.main(argc-1, argv+1);
    }

    std::cerr << "fluxengine: unrecognised command (try help)\n";
    return 1;
}
