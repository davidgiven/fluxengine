#include "globals.h"
#include "decoders.h"
#include "sector.h"
#include "image.h"
#include "amiga.h"
#include "crc.h"
#include "record.h"
#include <string.h>

static std::vector<uint8_t> deinterleave(const uint8_t*& input, size_t len)
{
    assert(!(len & 1));
    const uint8_t* odds = input;
    const uint8_t* evens = input + len/2;
    std::vector<uint8_t> output;
    output.reserve(len);

    for (int i=0; i<len/2; i++)
    {
        uint8_t y = *evens++;
        uint8_t x = *odds++;

        /* This is the 'Interleave bits with 64-bit multiply' technique from
         * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
         */
        uint16_t result =
            ((x * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 49) & 0x5555 |
            ((y * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 48) & 0xAAAA;
        
        output.push_back((uint8_t)(result >> 8));
        output.push_back((uint8_t)result);
    }

    input += len;
    return output;
}

std::vector<std::unique_ptr<Sector>> AmigaRecordParser::parseRecordsToSectors(
        const RecordVector& records) const
{
	int nextTrack = 0;
	int nextSector = 0;
    bool hasHeader = false;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
		const std::vector<uint8_t>& data = record->data;
        if (data.size() < 544)
            continue;

        const uint8_t* ptr = &data[4];

        std::vector<uint8_t> header = deinterleave(ptr, 4);
        std::vector<uint8_t> recoveryinfo = deinterleave(ptr, 16);
        std::vector<uint8_t> headerchecksum = deinterleave(ptr, 4);
        std::vector<uint8_t> datachecksum = deinterleave(ptr, 4);
        std::vector<uint8_t> databytes = deinterleave(ptr, 512);

        int track = header[1] >> 1;
        int head = header[1] & 1;
        auto sector = std::unique_ptr<Sector>(
            new Sector(Sector::OK, track, head, header[2], databytes));
        sectors.push_back(std::move(sector));
    }

    return sectors;
}
