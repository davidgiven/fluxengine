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

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static std::vector<uint8_t> decode(const std::vector<bool>& bits)
{
    BitAccumulator ba;

    auto ii = bits.begin();
    while (ii != bits.end())
    {
        uint8_t inputfifo = 0;
        for (size_t i=0; i<5; i++)
        {
            if (ii == bits.end())
                break;
            inputfifo = (inputfifo<<1) | *ii++;
        }

        ba.push(decode_data_gcr(inputfifo), 4);
    }

    return ba;
}

SectorVector Commodore64Decoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& rawbytes = decode(rawdata);

        if (rawbytes.size() == 0)
            continue;
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
		return 12;
    return 0;
}
