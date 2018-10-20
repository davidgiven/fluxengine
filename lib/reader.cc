#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "reader.h"
#include "fluxmap.h"
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

Fluxmap& Track::read()
{
    if (!_read)
    {
        reallyRead();
        _read = true;
    }
    return *_fluxmap.get();
}
    
void Track::forceReread()
{
    _read = false;
}

void CapturedTrack::reallyRead()
{
    std::cout << "read track " << track << " side " << side << ": " << std::flush;

    usbSeek(track);
    _fluxmap = usbRead(side, revolutions);
    std::cout << int(_fluxmap->duration()/1e6) << "ms in " << _fluxmap->bytes() << " bytes" << std::endl;
}

void FileTrack::reallyRead()
{
    Error() << "unsupported";
}

std::vector<std::unique_ptr<Track>> readTracks()
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

    std::vector<std::unique_ptr<Track>> tracks;
    for (int track=starttrack; track<=endtrack; track++)
    {
        for (int side=startside; side<=endside; side++)
        {
            std::unique_ptr<Track> t(
                basefilename.empty() ? (Track*)new CapturedTrack() : (Track*)new FileTrack());
            t->track = track;
            t->side = side;
            tracks.push_back(std::move(t));
        }
    }
    return tracks;
}
