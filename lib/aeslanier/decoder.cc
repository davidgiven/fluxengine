#include "globals.h"
#include "decoders.h"
#include "aeslanier.h"
#include "crc.h"
#include "fluxmap.h"
#include "sector.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>

/* This is actually M2FM, rather than MFM, but it our MFM/FM decoder copes fine with it. */

static std::vector<uint8_t> reverse_bits(const std::vector<uint8_t>& input)
{
    std::vector<uint8_t> output;
    for (uint8_t b : input)
        output.push_back(reverse_bits(b));
    return output;
}

SectorVector AesLanierDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& bytes = decodeFmMfm(rawdata);
        const auto& reversed = reverse_bits(bytes);

        if (reversed.size() < 0x103)
            continue;

        unsigned track = reversed[1];
        unsigned sectorid = reversed[2];

        /* Basic sanity checking. */

        if (track > 85)
            continue;
        if (sectorid > 35)
            continue;

        /* Check header 'checksum' (which seems far too simple to mean much). */

        {
            uint8_t wanted = reversed[3];
            uint8_t got = reversed[1] + reversed[2];
            if (wanted != got)
                continue;
        }

        /* Check data checksum, which also includes the header and is
         * significantly better. */

        uint16_t wanted = read_le16(&reversed[0x101]);
        uint16_t got = crc16ref(MODBUS_POLY_REF,
            &reversed[1], &reversed[1+AESLANIER_SECTOR_LENGTH]);
        int status = (wanted == got) ? Sector::OK : Sector::BAD_CHECKSUM;

        const std::vector<uint8_t> data(&reversed[1], &reversed[1+AESLANIER_SECTOR_LENGTH]);
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, 0, sectorid, data));
        sectors.push_back(std::move(sector));
	}

	return sectors;
}

nanoseconds_t AesLanierDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock() / 2;
}

int AesLanierDecoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffffffff;
    if (masked == AESLANIER_RECORD_SEPARATOR)
	 	return 16;
    return 0;
}
