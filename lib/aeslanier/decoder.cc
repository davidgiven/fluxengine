#include "globals.h"
#include "decoders.h"
#include "aeslanier.h"
#include "crc.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "sector.h"
#include "bytes.h"
#include "record.h"
#include "fmt/format.h"
#include <string.h>

static const FluxPattern SECTOR_PATTERN(32, AESLANIER_RECORD_SEPARATOR);

/* This is actually M2FM, rather than MFM, but it our MFM/FM decoder copes fine with it. */

static Bytes reverse_bits(const Bytes& input)
{
    Bytes output;
    ByteWriter bw(output);

    for (uint8_t b : input)
        bw.write_8(reverse_bits(b));
    return output;
}

nanoseconds_t AesLanierDecoder::findSector(FluxmapReader& fmr, Track& track)
{
    return fmr.seekToPattern(SECTOR_PATTERN);
}

void AesLanierDecoder::decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector)
{
    /* Skip ID mark. */

    fmr.readRawBits(16, sector.clock);

    const auto& rawbits = fmr.readRawBits(AESLANIER_RECORD_SIZE*16, sector.clock);
    const auto& bytes = decodeFmMfm(rawbits).slice(0, AESLANIER_RECORD_SIZE);
    const auto& reversed = reverse_bits(bytes);

    sector.logicalTrack = reversed[1];
    sector.logicalSide = 0;
    sector.logicalSector = reversed[2];

    /* Check header 'checksum' (which seems far too simple to mean much). */

    {
        uint8_t wanted = reversed[3];
        uint8_t got = reversed[1] + reversed[2];
        if (wanted != got)
            return;
    }

    /* Check data checksum, which also includes the header and is
        * significantly better. */

    sector.data = reversed.slice(1, AESLANIER_SECTOR_LENGTH);
    uint16_t wanted = reversed.reader().seek(0x101).read_le16();
    uint16_t got = crc16ref(MODBUS_POLY_REF, sector.data);
    sector.status = (wanted == got) ? Sector::OK : Sector::BAD_CHECKSUM;
}
