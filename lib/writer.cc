#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include "sql.h"
#include "protocol.h"
#include "usb.h"
#include "fmt/format.h"
#include <regex>

static const std::regex DEST_REGEX("([^:]*)"
                                     "(?::t=([0-9]+)(?:-([0-9]+))?)?"
                                     "(?::s=([0-9]+)(?:-([0-9]+))?)?");

static StringFlag dest(
    { "--dest", "-d" },
    "destination for data",
    "");

static std::string basefilename;
static int starttrack = 0;
static int endtrack = 79;
static int startside = 0;
static int endside = 1;
static sqlite3* outdb;

void setWriterDefaults(int minTrack, int maxTrack, int minSide, int maxSide)
{
	starttrack = minTrack;
	endtrack = maxTrack;
	startside = minSide;
	endside = maxSide;
}

void writeTracks(const std::function<Fluxmap(int track, int side)> producer)
{
    auto f = dest.value();
    std::smatch match;
    if (!std::regex_match(f, match, DEST_REGEX))
        Error() << "invalid destination specifier '" << dest.value() << "'";
    
    basefilename = match[1];
    if (match[2].length() != 0)
        starttrack = endtrack = std::stoi(match[2]);
    if (match[3].length() != 0)
        endtrack = std::stoi(match[3]);
    if (match[4].length() != 0)
        startside = endside = std::stoi(match[4]);
    if (match[5].length() != 0)
        endside = std::stoi(match[5]);

    std::cout << "Writing to: "
              << (basefilename.empty() ? "a real floppy disk" : basefilename) << std::endl
              << "Tracks:       "
              << starttrack << " to " << endtrack << " inclusive" << std::endl
              << "Sides:        "
              << startside << " to " << endside << " inclusive" << std::endl;

	if (!basefilename.empty())
	{
		outdb = sqlOpen(basefilename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		sqlPrepareFlux(outdb);
		sqlStmt(outdb, "BEGIN;");
		atexit([]()
			{
				sqlStmt(outdb, "COMMIT;");
				sqlClose(outdb);
			}
		);
	}

    for (int track=starttrack; track<=endtrack; track++)
    {
        for (int side=startside; side<=endside; side++)
        {
			std::cout << fmt::format("{0:>3}.{1}: ", track, side) << std::flush;
			Fluxmap fluxmap = producer(track, side);
			fluxmap.precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
			if (outdb)
				sqlWriteFlux(outdb, track, side, fluxmap);
			else
			{
				usbSeek(track);
				usbWrite(side, fluxmap);
			}
			std::cout << fmt::format("{0} ms in {1} bytes", int(fluxmap.duration()/1e6), fluxmap.bytes()) << std::endl;
        }
    }
}

void fillBitmapTo(std::vector<bool>& bitmap,
	unsigned& cursor, unsigned terminateAt,
	const std::vector<bool>& pattern)
{
	while (cursor < terminateAt)
	{
		for (bool b : pattern)
		{
			if (cursor < bitmap.size())
				bitmap[cursor++] = b;
		}
	}
}

