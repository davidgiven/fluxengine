#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "macintosh.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"
#include "fmt/format.h"
#include <ctype.h>

FlagGroup macintoshEncoderFlags;

std::unique_ptr<Fluxmap> MacintoshEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= MAC_TRACKS_PER_DISK))
		return std::unique_ptr<Fluxmap>();

	return std::unique_ptr<Fluxmap>();
}

