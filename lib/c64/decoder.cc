#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "c64.h"
#include "crc.h"
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
    unsigned nextSector;
    unsigned nextTrack;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& bytes = decode(rawdata);

        if (bytes.size() == 0)
            continue;

        switch (bytes[0])
        {
            case 8: /* sector record */
            {
                headerIsValid = false;
                if (bytes.size() < 6)
                    break;

                uint8_t checksum = bytes[1];
                nextSector = bytes[2];
                nextTrack = bytes[3];
                if (checksum != xorBytes(&bytes[2], &bytes[6]))
                    break;

                headerIsValid = true;
                break;
            }
            
            case 7: /* data record */
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;
                if (bytes.size() < 258)
                    break;

                uint8_t checksum = xorBytes(&bytes[1], &bytes[257]);
                int status = (checksum == bytes[257]) ? Sector::OK : Sector::BAD_CHECKSUM;

                auto sector = std::unique_ptr<Sector>(
					new Sector(status, nextTrack, 0, nextSector,
                        std::vector<uint8_t>(&bytes[1], &bytes[257])));
                sectors.push_back(std::move(sector));
                break;
            }
        }
	}

	return sectors;
}

int Commodore64Decoder::recordMatcher(uint64_t fifo) const
{
    uint16_t masked = fifo & 0xffff;
    if (masked == C64_RECORD_SEPARATOR)
		return 4;
    return 0;
}
