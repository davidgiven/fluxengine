#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "macintosh.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

SectorVector MacintoshDecoder::decodeToSectors(const RawRecordVector& rawRecords)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
	}

	return sectors;
}

int MacintoshDecoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffffff;
    if ((masked == MAC_SECTOR_RECORD) || (masked == MAC_DATA_RECORD))
		return 24;
    return 0;
}
