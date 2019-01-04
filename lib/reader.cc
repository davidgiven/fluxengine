#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "fmt/format.h"
#include <regex>

static const std::regex SOURCE_REGEX("([^:]*)"
                                     "(?::t=([0-9]+)(?:-([0-9]+))?)?"
                                     "(?::s=([0-9]+)(?:-([0-9]+))?)?");

static StringFlag source(
    { "--source", "-s" },
    "source for data",
    "");

static StringFlag destination(
    { "--write-flux", "-f" },
    "write the raw magnetic flux to this file",
    "");

static SettableFlag justRead(
    { "--just-read", "-R" },
    "just read the disk but do no further processing");

static IntFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1);

static std::string basefilename;
static int starttrack = 0;
static int endtrack = 79;
static int startside = 0;
static int endside = 1;
static sqlite3* indb;
static sqlite3* outdb;

void setReaderDefaults(int minTrack, int maxTrack, int minSide, int maxSide)
{
	starttrack = minTrack;
	endtrack = maxTrack;
	startside = minSide;
	endside = maxSide;
}

Fluxmap& ReaderTrack::read()
{
    if (!_read)
    {
        std::cout << fmt::format("{0:>3}.{1}: ", track, side) << std::flush;
        reallyRead();
        std::cout << fmt::format("{0} ms in {1} bytes", int(_fluxmap->duration()/1e6), _fluxmap->bytes()) << std::endl;
        _read = true;

		if (outdb)
			sqlWriteFlux(outdb, track, side, *_fluxmap);
    }

    return *_fluxmap.get();
}
    
void ReaderTrack::forceReread()
{
    _read = false;
}

void CapturedReaderTrack::reallyRead()
{
    usbSeek(track);
    _fluxmap = usbRead(side, revolutions);
}

void FileReaderTrack::reallyRead()
{
    if (!indb)
	{
        indb = sqlOpen(basefilename, SQLITE_OPEN_READONLY);
		atexit([]() { sqlClose(indb); });
	}
    _fluxmap = sqlReadFlux(indb, track, side);
}

std::vector<std::unique_ptr<ReaderTrack>> readTracks()
{
    auto f = source.value();
    std::smatch match;
    if (!std::regex_match(f, match, SOURCE_REGEX))
        Error() << "invalid source specifier '" << source.value() << "'";
    
    basefilename = match[1];
    if (match[2].length() != 0)
        starttrack = endtrack = std::stoi(match[2]);
    if (match[3].length() != 0)
        endtrack = std::stoi(match[3]);
    if (match[4].length() != 0)
        startside = endside = std::stoi(match[4]);
    if (match[5].length() != 0)
        endside = std::stoi(match[5]);

    std::cout << "Reading from: "
              << (basefilename.empty() ? "a real floppy disk" : basefilename) << std::endl
              << "Tracks:       "
              << starttrack << " to " << endtrack << " inclusive" << std::endl
              << "Sides:        "
              << startside << " to " << endside << " inclusive" << std::endl;

	if (!destination.value().empty())
	{
		outdb = sqlOpen(destination, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		std::cout << "Writing a copy of the flux to " << destination.value() << std::endl;
		sqlPrepareFlux(outdb);
		sqlStmt(outdb, "BEGIN;");
		atexit([]()
			{
				sqlStmt(outdb, "COMMIT;");
				sqlClose(outdb);
			}
		);
	}

    std::vector<std::unique_ptr<ReaderTrack>> tracks;
    for (int track=starttrack; track<=endtrack; track++)
    {
        for (int side=startside; side<=endside; side++)
        {
            std::unique_ptr<ReaderTrack> t(
                basefilename.empty()
					? (ReaderTrack*)new CapturedReaderTrack()
					: (ReaderTrack*)new FileReaderTrack());
            t->track = track;
            t->side = side;
            tracks.push_back(std::move(t));
        }
    }

	if (justRead)
	{
		for (auto& track : tracks)
			track->read();
		std::cout << "--just-read specified, terminating without further processing" << std::endl;
		exit(0);
	}
				
    return tracks;
}
