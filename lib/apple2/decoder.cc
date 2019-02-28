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
 * and R. Belmont: https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap_dsk35.cpp
 */
static std::vector<uint8_t> decode_crazy_data(const uint8_t* inp, int& status)
{
    std::vector<uint8_t> output;

    static const int LOOKUP_LEN = MAC_SECTOR_LENGTH / 3;

    uint8_t b1[LOOKUP_LEN + 1];
    uint8_t b2[LOOKUP_LEN + 1];
    uint8_t b3[LOOKUP_LEN + 1];

    for (int i=0; i<=LOOKUP_LEN; i++)
    {
        uint8_t w4 = *inp++;
        uint8_t w1 = *inp++;
        uint8_t w2 = *inp++;
        uint8_t w3 = (i != 174) ? *inp++ : 0;

        b1[i] = (w1 & 0x3F) | ((w4 << 2) & 0xC0);
        b2[i] = (w2 & 0x3F) | ((w4 << 4) & 0xC0);
        b3[i] = (w3 & 0x3F) | ((w4 << 6) & 0xC0);
    }

    /* Copy from the user's buffer to our buffer, while computing
     * the three-byte data checksum. */

    uint32_t c1 = 0;
    uint32_t c2 = 0;
    uint32_t c3 = 0;
    unsigned count = 0;
    for (;;)
    {
        c1 = (c1 & 0xFF) << 1;
        if (c1 & 0x0100)
            c1++;

        uint8_t val = b1[count] ^ c1;
        c3 += val;
        if (c1 & 0x0100)
        {
            c3++;
            c1 &= 0xFF;
        }
        output.push_back(val);

        val = b2[count] ^ c3;
        c2 += val;
        if (c3 > 0xFF)
        {
            c2++;
            c3 &= 0xFF;
        }
        output.push_back(val);

        if (output.size() == 524)
            break;

        val = b3[count] ^ c2;
        c1 += val;
        if (c2 > 0xFF)
        {
            c1++;
            c2 &= 0xFF;
        }
        output.push_back(val);
        count++;
    }

    uint8_t c4 = ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
    c1 &= 0x3f;
    c2 &= 0x3f;
    c3 &= 0x3f;
    c4 &= 0x3f;
    uint8_t g4 = *inp++;
    uint8_t g3 = *inp++;
    uint8_t g2 = *inp++;
    uint8_t g1 = *inp++;
    if ((g4 == c4) && (g3 == c3) && (g2 == c2) && (g1 == c1))
        status = Sector::OK;

    return output;
}

uint8_t decode_side(uint8_t side)
{
    /* Mac disks, being weird, use the side byte to encode both the side (in
     * bit 5) and also whether we're above track 0x3f (in bit 6).
     */

    return !!(side & 0x40);
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
            case MAC_SECTOR_RECORD:
            {
                uint8_t volume = combine(read_be16(&rawbytes[3]));
                nextTrack = combine(read_be16(&rawbytes[5]));
                nextSector = combine(read_be16(&rawbytes[7]));
                uint8_t checksum = combine(read_be16(&rawbytes[9]));
                headerIsValid = checksum == (volume ^ nextTrack ^ nextSector);
                break;
            }

            case MAC_DATA_RECORD:
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;

                uint8_t inputbuffer[MAC_SECTOR_LENGTH * 8/6 + 5] = {};
                for (unsigned i=0; i<sizeof(inputbuffer); i++)
                {
                    auto p = rawbytes.begin() + 4 + i;
                    if (p > rawbytes.end())
                        break;
                    
                    inputbuffer[i] = decode_data_gcr(*p);
                }
                    
                int status = Sector::BAD_CHECKSUM;
                auto data = decode_crazy_data(inputbuffer, status);

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
    if ((masked == MAC_SECTOR_RECORD) || (masked == MAC_DATA_RECORD))
		return 24;
    return 0;
}
