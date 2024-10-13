#include "lib/core/globals.h"
#include "lib/config/proto.h"
#include "fmt/format.h"

typedef int command_cb(int agrc, const char* argv[]);

extern command_cb mainAnalyseDriveResponse;
extern command_cb mainAnalyseLayout;
extern command_cb mainFormat;
extern command_cb mainGetDiskInfo;
extern command_cb mainGetFile;
extern command_cb mainGetFileInfo;
extern command_cb mainInspect;
extern command_cb mainLs;
extern command_cb mainMerge;
extern command_cb mainMkDir;
extern command_cb mainMv;
extern command_cb mainPutFile;
extern command_cb mainRawRead;
extern command_cb mainRawWrite;
extern command_cb mainRead;
extern command_cb mainRm;
extern command_cb mainRpm;
extern command_cb mainSeek;
extern command_cb mainTestBandwidth;
extern command_cb mainTestDevices;
extern command_cb mainTestVoltages;
extern command_cb mainWrite;

struct Command
{
    std::string name;
    command_cb* main;
    std::string help;
};

static command_cb mainAnalyse;
static command_cb mainTest;

// clang-format off
static std::vector<Command> commands =
{
    { "inspect",           mainInspect,           "Low-level analysis and inspection of a disk." },
	{ "analyse",           mainAnalyse,           "Disk and drive analysis tools." },
    { "read",              mainRead,              "Reads a disk, producing a sector image.", },
    { "write",             mainWrite,             "Writes a sector image to a disk.", },
	{ "format",            mainFormat,            "Format a disk and make a file system on it.", },
	{ "rawread",           mainRawRead,           "Reads raw flux from a disk. Warning: you can't use this to copy disks.", },
    { "rawwrite",          mainRawWrite,          "Writes a flux file to a disk. Warning: you can't use this to copy disks.", },
	{ "merge",             mainMerge,             "Merge together multiple flux files.", },
	{ "getdiskinfo",       mainGetDiskInfo,       "Read volume metadata off a disk (or image).", },
	{ "ls",                mainLs,                "Show files on disk (or image).", },
	{ "mv",                mainMv,                "Rename a file on a disk (or image).", },
	{ "rm",                mainRm,                "Deletes a file (or directory) off a disk (or image).", },
	{ "getfile",           mainGetFile,           "Read a file off a disk (or image).", },
	{ "getfileinfo",       mainGetFileInfo,       "Read file metadata off a disk (or image).", },
	{ "putfile",           mainPutFile,           "Write a file to disk (or image).", },
	{ "mkdir",             mainMkDir,             "Create a directory on disk (or image).", },
    { "rpm",               mainRpm,               "Measures the disk rotational speed.", },
    { "seek",              mainSeek,              "Moves the disk head.", },
    { "test",              mainTest,              "Various testing commands.", },
};

static std::vector<Command> analysables =
{
	{ "driveresponse", mainAnalyseDriveResponse, "Measures the drive's ability to read and write pulses.", },
	{ "layout",        mainAnalyseLayout,        "Produces a visualisation of the track/sector layout.", },
};

static std::vector<Command> testables =
{
    { "bandwidth",     mainTestBandwidth, "Measures your USB bandwidth.", },
    { "devices",       mainTestDevices, "Displays all detected devices.", },
    { "voltages",      mainTestVoltages,  "Measures the FDD bus voltages.", },
};
// clang-format on

static void extendedHelp(
    std::vector<Command>& subcommands, const std::string& command)
{
    std::cout << "fluxengine: syntax: fluxengine " << command
              << " <format> [<flags>...]\n"
                 "These subcommands are supported:\n";

    for (Command& c : subcommands)
        std::cout << "  " << c.name << ": " << c.help << "\n";

    exit(0);
}

static int mainExtended(std::vector<Command>& subcommands,
    const std::string& command,
    int argc,
    const char* argv[])
{
    if (argc == 1)
        extendedHelp(subcommands, command);

    std::string format = argv[1];
    if (format == "--help")
        extendedHelp(subcommands, command);

    for (Command& c : subcommands)
    {
        if (format == c.name)
            return c.main(argc - 1, argv + 1);
    }

    std::cerr << "fluxengine: unrecognised format (try --help)\n";
    return 1;
}

static int mainAnalyse(int argc, const char* argv[])
{
    return mainExtended(analysables, "analyse", argc, argv);
}

static int mainTest(int argc, const char* argv[])
{
    return mainExtended(testables, "test", argc, argv);
}

static void globalHelp()
{
    std::cout << "fluxengine: syntax: fluxengine <command> [<flags>...]\n"
                 "Try one of these commands:\n";

    for (Command& c : commands)
        std::cout << "  " << c.name << ": " << c.help << "\n";

    exit(0);
}

void showProfiles(const std::string& command,
    const std::map<std::string, const ConfigProto*>& profiles)
{
    std::cout << "syntax: fluxengine " << command
              << " <profile> [<extensions...>] [<options>...]\n"
                 "Use --help for option help.\n"
                 "Available profiles include:\n";

    for (const auto& it : profiles)
    {
        const auto& c = *it.second;
        if (!c.is_extension())
            std::cout << fmt::format("  {}: {}\n", it.first, c.shortname());
    }

    std::cout << "Available profile options include:\n";

    for (const auto& it : profiles)
    {
        const auto& c = *it.second;
        if (c.is_extension() && (it.first[0] != '_'))
            std::cout << fmt::format("  {}: {}\n", it.first, c.comment());
    }

    std::cout << "Profiles and extensions may also be textpb files .\n";
    exit(1);
}

int main(int argc, const char* argv[])
{
    if (argc == 1)
        globalHelp();

    std::string command = argv[1];
    if (command == "--help")
        globalHelp();

    for (Command& c : commands)
    {
        if (command == c.name)
        {
            try
            {
                return c.main(argc - 1, argv + 1);
            }
            catch (const ErrorException& e)
            {
                fmt::print(stderr, "Error: {}\n", e.message);
                exit(1);
            }
        }
    }

    std::cerr << "fluxengine: unrecognised command (try --help)\n";
    return 1;
}
