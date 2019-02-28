#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "apple2.h"
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

/* This is extremely inspired by the MESS implementation, written by Nathan Woods
 * and R. Belmont: https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib/formats/ap2_dsk.cpp
 */
static std::vector<uint8_t> decode_crazy_data(const uint8_t* inp, int& status)
{
    std::vector<uint8_t> output(APPLE2_SECTOR_LENGTH);

    uint8_t checksum = 0;
    for (unsigned i = 0; i < APPLE2_ENCODED_SECTOR_LENGTH; i++)
    {
        uint8_t b = decode_data_gcr(*inp++);
        uint8_t newvalue = b ^ checksum;

        if (i >= 0x56)
        {
            /* 6 bit */
            output[i - 0x56] |= (newvalue << 2);
        }
        else
        {
            /* 3 * 2 bit */
            output[i + 0x00] = ((newvalue >> 1) & 0x01) | ((newvalue << 1) & 0x02);
            output[i + 0x56] = ((newvalue >> 3) & 0x01) | ((newvalue >> 1) & 0x02);
            if ((i + 0xAC) < APPLE2_SECTOR_LENGTH)
                output[i + 0xAC] = ((newvalue >> 5) & 0x01) | ((newvalue >> 3) & 0x02);
        }
        checksum = newvalue;
    }

    checksum &= 0x3f;
    uint8_t wantedchecksum = decode_data_gcr(*inp);
    status = (checksum == wantedchecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    return output;
}

uint8_t combine(uint16_t word)
{
    return word & (word >> 7);
}

SectorVector Apple2Decoder::decodeToSectors(
        const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    int nextTrack;
    int nextSector;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const std::vector<bool>& rawdata = rawrecord->data;
        const std::vector<uint8_t>& rawbytes = toBytes(rawdata);

        if (rawbytes.size() < 8)
            continue;

        uint32_t signature = read_be24(&rawbytes[0]);
        switch (signature)
        {
            case APPLE2_SECTOR_RECORD:
            {
                uint8_t volume = combine(read_be16(&rawbytes[3]));
                nextTrack = combine(read_be16(&rawbytes[5]));
                nextSector = combine(read_be16(&rawbytes[7]));
                uint8_t checksum = combine(read_be16(&rawbytes[9]));
                headerIsValid = checksum == (volume ^ nextTrack ^ nextSector);
                break;
            }

            case APPLE2_DATA_RECORD:
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;
                    
                std::vector<uint8_t> clippedbytes(rawbytes);
                clippedbytes.resize(APPLE2_ENCODED_SECTOR_LENGTH + 5);
                int status = Sector::BAD_CHECKSUM;
                auto data = decode_crazy_data(&clippedbytes[4], status);

                auto sector = std::unique_ptr<Sector>(
                    new Sector(status, nextTrack, 0, nextSector, data));
                sectors.push_back(std::move(sector));
                break;
            }
        }
	}

	return sectors;
}

int Apple2Decoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffffff;
    if ((masked == APPLE2_SECTOR_RECORD) || (masked == APPLE2_DATA_RECORD))
		return 24;
    return 0;
}
