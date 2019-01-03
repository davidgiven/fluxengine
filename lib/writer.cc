#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include <regex>

static const std::regex DEST_REGEX("([^:]*)"
                                     "(?::t=([0-9]+)(?:-([0-9]+))?)?"
                                     "(?::s=([0-9]+)(?:-([0-9]+))?)?");

static StringFlag destination(
    { "--dest", "-d" },
    "destination for data",
    "");

void writeTrack(int track, int side, const Fluxmap& fluxmap)
{
	Error() << "not implemented yet";
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

