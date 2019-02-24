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
    uint64_t masked = fifo & 0xffffffffffffULL;
    if (masked == 0)
		return 64;
    return 0;
}
