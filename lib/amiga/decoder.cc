#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "amiga.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

/* 
 * Amiga disks use MFM but it's not quite the same as IBM MFM. They only use
 * a single type of record with a different marker byte.
 * 
 * See the big comment in the IBM MFM decoder for the gruesome details of how
 * MFM works.
 */
         
static std::vector<uint8_t> deinterleave(std::vector<uint8_t>::const_iterator& input, size_t len)
{
    assert(!(len & 1));
    const uint8_t* odds = &input[0];
    const uint8_t* evens = &input[len/2];
    std::vector<uint8_t> output;
    output.reserve(len);

    for (size_t i=0; i<len/2; i++)
    {
        uint8_t o = *odds++;
        uint8_t e = *evens++;

        /* This is the 'Interleave bits with 64-bit multiply' technique from
         * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
         */
        uint16_t result =
            (((e * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 49) & 0x5555) |
            (((o * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 48) & 0xAAAA);
        
        output.push_back((uint8_t)(result >> 8));
        output.push_back((uint8_t)result);
    }

    input += len;
    return output;
}

static uint32_t checksum(std::vector<uint8_t>::const_iterator input, size_t len)
{
    uint32_t checksum = 0;

    assert((len & 3) == 0);
    while (len != 0)
    {
        checksum ^= read_be32(input);
        input += 4;
        len -= 4;
    }

    return checksum & 0x55555555;
}

SectorVector AmigaDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& rawbytes = toBytes(rawdata);
        const auto& bytes = decodeFmMfm(rawdata);

        if (bytes.size() < 544)
            continue;

        auto ptr = bytes.begin() + 4;

        std::vector<uint8_t> header = deinterleave(ptr, 4);
        std::vector<uint8_t> recoveryinfo = deinterleave(ptr, 16);

        uint32_t wantedheaderchecksum = read_be32(deinterleave(ptr, 4));
        uint32_t gotheaderchecksum = checksum(rawbytes.begin() + 8, 40);
        if (gotheaderchecksum != wantedheaderchecksum)
            continue;

        uint32_t wanteddatachecksum = read_be32(deinterleave(ptr, 4));
        uint32_t gotdatachecksum = checksum(rawbytes.begin() + 64, 1024);
        int status = (gotdatachecksum == wanteddatachecksum) ? Sector::OK : Sector::BAD_CHECKSUM;

        std::vector<uint8_t> databytes = deinterleave(ptr, 512);
        unsigned track = header[1] >> 1;
        unsigned side = header[1] & 1;
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, side, header[2], databytes));
        sectors.push_back(std::move(sector));
	}

	return sectors;
}

nanoseconds_t AmigaDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock() / 2;
}

int AmigaDecoder::recordMatcher(uint64_t fifo) const
{
    uint64_t masked = fifo & 0xffffffffffffULL;
    if (masked == AMIGA_SECTOR_RECORD)
		return 64;
    return 0;
}
