#include "globals.h"

typedef int command_cb(int agrc, const char* argv[]);

extern command_cb mainErase;
extern command_cb mainConvertCwfToFlux;
extern command_cb mainConvertFluxToAu;
extern command_cb mainConvertFluxToVcd;
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
extern command_cb mainReadMac;
extern command_cb mainReadMx;
extern command_cb mainReadQd;
extern command_cb mainReadVictor9K;
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

static command_cb mainRead;
static command_cb mainWrite;
static command_cb mainConvert;

static std::vector<Command> commands =
{
    { "erase",             mainErase,             "Permanently but rapidly erases some or all of a disk." },
    { "convert",           mainConvert,           "Converts various types of data file.", },
    { "inspect",           mainInspect,           "Low-level analysis and inspection of a disk." },
    { "read",              mainRead,              "Reads a disk, producing a sector image.", },
    { "rpm",               mainRpm,               "Measures the disk rotational speed.", },
    { "seek",              mainSeek,              "Moves the disk head.", },
    { "testbulktransport", mainTestBulkTransport, "Measures your USB bandwidth.", },
    { "upgradefluxfile",   mainUpgradeFluxFile,   "Upgrades a flux file from a previous version of this software.", },
    { "write",             mainWrite,             "Writes a sector image to a disk.", },
    { "writeflux",         mainWriteFlux,         "Writes a raw flux file. Warning: you can't use this to copy disks.", },
    { "writetestpattern",  mainWriteTestPattern,  "Writes a machine-generated test pattern to a disk.", },
};

static std::vector<Command> readables =
{
    { "adfs",          mainReadADFS,      "Reads Acorn ADFS disks.", },
    { "aeslanier",     mainReadAESLanier, "Reads AES Lanier disks.", },
    { "amiga",         mainReadAmiga,     "Reads Commodore Amiga disks.", },
    { "ampro",         mainReadAmpro,     "Reads Ampro disks.", },
    { "apple2",        mainReadApple2,    "Reads Apple II disks.", },
    { "brother",       mainReadBrother,   "Reads 120kB and 240kB Brother word processor disks.", },
    { "c64",           mainReadC64,       "Reads Commodore 64 disks.", },
    { "dfs",           mainReadDFS,       "Reads Acorn DFS disks.", },
    { "f85",           mainReadF85,       "Reads Durango F85 disks.", },
    { "fb100",         mainReadFB100,     "Reads FB100 disks.", },
    { "ibm",           mainReadIBM,       "Reads the ubiquitous IBM format disks.", },
    { "mac",           mainReadMac,       "Reads Apple Macintosh disks.", },
    { "mx",            mainReadMx,        "Reads MX disks.", },
    { "qd",            mainReadQd,        "Reads QuickDisk disks.", },
    { "victor9k",      mainReadVictor9K,  "Reads Victor 9000 disks.", },
    { "zilogmcz",      mainReadZilogMCZ,  "Reads Zilog MCZ disks.", },
};

static std::vector<Command> writeables =
{
    { "brother",       mainWriteBrother,  "Writes 120kB and 240kB Brother word processor disks.", },
};

static std::vector<Command> convertables =
{
    { "cwftoflux",     mainConvertCwfToFlux, "Converts CatWeasel stream files to flux.", },
    { "fluxtoau",      mainConvertFluxToAu,  "Converts (one track of a) flux file to an .au audio file.", },
    { "fluxtovcd",     mainConvertFluxToVcd, "Converts (one track of a) flux file to a VCD file.", },
};

static void extendedHelp(std::vector<Command>& subcommands, const std::string& command)
{
    std::cout << "fluxengine: syntax: fluxengine " << command << " <format> [<flags>...]\n"
                 "These subcommands are supported:\n";

    for (Command& c : subcommands)
        std::cout << "  " << c.name << ": " << c.help << "\n";

    exit(0);
}

static int mainExtended(std::vector<Command>& subcommands, const std::string& command,
         int argc, const char* argv[])
{
    if (argc == 1)
        extendedHelp(subcommands, command);

    std::string format = argv[1];
    if (format == "--help")
        extendedHelp(subcommands, command);

    for (Command& c : subcommands)
    {
        if (format == c.name)
            return c.main(argc-1, argv+1);
    }

    std::cerr << "fluxengine: unrecognised format (try --help)\n";
    return 1;
}

static int mainRead(int argc, const char* argv[])
{ return mainExtended(readables, "read", argc, argv); }

static int mainWrite(int argc, const char* argv[])
{ return mainExtended(writeables, "write", argc, argv); }

static int mainConvert(int argc, const char* argv[])
{ return mainExtended(convertables, "convert", argc, argv); }

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
    if (command == "--help")
        help();

    for (Command& c : commands)
    {
        if (command == c.name)
            return c.main(argc-1, argv+1);
    }

    std::cerr << "fluxengine: unrecognised command (try --help)\n";
    return 1;
}
