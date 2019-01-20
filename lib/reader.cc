#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "dataspec.h"
#include "fmt/format.h"

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":t=0-79:s=0-1");

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

static sqlite3* indb;
static sqlite3* outdb;

void setReaderDefaultSource(const std::string& source)
{
    ::source.set(source);
}

std::unique_ptr<Fluxmap> ReaderTrack::read()
{
    std::cout << fmt::format("{0:>3}.{1}: ", track, side) << std::flush;
    std::unique_ptr<Fluxmap> fluxmap = reallyRead();
    std::cout << fmt::format(
        "{0} ms in {1} bytes", int(fluxmap->duration()/1e6), fluxmap->bytes()) << std::endl;

    if (outdb)
        sqlWriteFlux(outdb, track, side, *fluxmap);

    return fluxmap;
}
    
std::unique_ptr<Fluxmap> CapturedReaderTrack::reallyRead()
{
    usbSeek(track);
    return usbRead(side, revolutions);
}

std::unique_ptr<Fluxmap> FileReaderTrack::reallyRead()
{
    if (!indb)
	{
        indb = sqlOpen(source.value.filename, SQLITE_OPEN_READONLY);
		atexit([]() { sqlClose(indb); });
	}
    return sqlReadFlux(indb, track, side);
}

std::vector<std::unique_ptr<ReaderTrack>> readTracks()
{
    const DataSpec& dataSpec = source.value;

    std::cout << "Reading from: " << dataSpec << std::endl;

	if (!destination.value.empty())
	{
		outdb = sqlOpen(destination, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		std::cout << "Writing a copy of the flux to " << destination.value << std::endl;
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
    for (const auto& location : dataSpec.locations)
    {
        std::unique_ptr<ReaderTrack> t(
            dataSpec.filename.empty()
                ? (ReaderTrack*)new CapturedReaderTrack()
                : (ReaderTrack*)new FileReaderTrack());
        t->track = location.track;
        t->side = location.side;
        tracks.push_back(std::move(t));
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
