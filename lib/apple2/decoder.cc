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
static Bytes decode_crazy_data(const uint8_t* inp, int& status)
{
    Bytes output(APPLE2_SECTOR_LENGTH);

    uint8_t checksum = 0;
    for (unsigned i = 0; i < APPLE2_ENCODED_SECTOR_LENGTH; i++)
    {
        checksum ^= decode_data_gcr(*inp++);

        if (i >= 86)
        {
            /* 6 bit */
            output[i - 86] |= (checksum << 2);
        }
        else
        {
            /* 3 * 2 bit */
            output[i + 0] = ((checksum >> 1) & 0x01) | ((checksum << 1) & 0x02);
            output[i + 86] = ((checksum >> 3) & 0x01) | ((checksum >> 1) & 0x02);
            if ((i + 172) < APPLE2_SECTOR_LENGTH)
                output[i + 172] = ((checksum >> 5) & 0x01) | ((checksum >> 3) & 0x02);
        }
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
        const RawRecordVector& rawRecords, unsigned, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    int nextTrack;
    int nextSector;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const std::vector<bool>& rawdata = rawrecord->data;
        const Bytes& rawbytes = toBytes(rawdata);
        ByteReader br(rawbytes);

        if (rawbytes.size() < 8)
            continue;

        uint32_t signature = br.read_be24();
        switch (signature)
        {
            case APPLE2_SECTOR_RECORD:
            {
                uint8_t volume = combine(br.read_be16());
                nextTrack = combine(br.read_be16());
                nextSector = combine(br.read_be16());
                uint8_t checksum = combine(br.read_be16());
                headerIsValid = checksum == (volume ^ nextTrack ^ nextSector);
                break;
            }

            case APPLE2_DATA_RECORD:
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;
                    
                Bytes clipped_bytes = rawbytes.slice(0, APPLE2_ENCODED_SECTOR_LENGTH + 5);
                int status = Sector::BAD_CHECKSUM;
                auto data = decode_crazy_data(&clipped_bytes[3], status);

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
