#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "c64.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

SectorVector Commodore64Decoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
	}

	return sectors;
}

nanoseconds_t Commodore64Decoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock();
}

int Commodore64Decoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffffff;
    if ((masked == C64_SECTOR_RECORD) || (masked == C64_DATA_RECORD))
		return 24;
    return 0;
}
