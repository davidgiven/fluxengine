#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "victor9k.h"
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

        uint8_t decoded = decode_data_gcr(inputfifo);
        ba.push(decoded, 4);
    }

    return ba;
}

SectorVector Victor9kDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    unsigned nextSector;
    unsigned nextTrack;
    unsigned nextSide;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& bytes = decode(rawdata);

        if (bytes.size() == 0)
            continue;

        switch (bytes[0])
        {
            case 7: /* sector record */
            {
                headerIsValid = false;
                if (bytes.size() < 6)
                    break;

                uint8_t rawTrack = bytes[1];
                nextSector = bytes[2];
                uint8_t gotChecksum = bytes[3];

                nextTrack = rawTrack & 0x7f;
                nextSide = rawTrack >> 7;
                uint8_t wantChecksum = sumBytes(&bytes[1], &bytes[3]);
                if (wantChecksum != gotChecksum)
                    break;
                if ((nextSector > 20) || (nextTrack > 85) || (nextSide > 1))
                    break;

                headerIsValid = true;
                break;
            }
            
            case 8: /* data record */
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;
                if (bytes.size() < VICTOR9K_SECTOR_LENGTH+3)
                    break;

                uint16_t gotChecksum = sumBytes(&bytes[1], &bytes[VICTOR9K_SECTOR_LENGTH+1]);
                uint16_t wantChecksum = read_le16(&bytes[VICTOR9K_SECTOR_LENGTH+1]);
                int status = (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;

                auto sector = std::unique_ptr<Sector>(
					new Sector(status, nextTrack, 0, nextSector,
                        std::vector<uint8_t>(&bytes[1], &bytes[VICTOR9K_SECTOR_LENGTH+1])));
                sectors.push_back(std::move(sector));
                break;
            }
        }
	}

	return sectors;
}

int Victor9kDecoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xfffff;
    if ((masked == VICTOR9K_SECTOR_RECORD) || (masked == VICTOR9K_DATA_RECORD))
		return 9;
    return 0;
}
