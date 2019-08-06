#include "globals.h"
#include "decoders/decoders.h"
#include "aeslanier.h"
#include "crc.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
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

AbstractDecoder::RecordType AesLanierDecoder::advanceToNextRecord()
{
    _sector->clock = _fmr->seekToPattern(SECTOR_PATTERN);
    if (_fmr->eof() || !_sector->clock)
        return UNKNOWN_RECORD;
    return SECTOR_RECORD;
}

void AesLanierDecoder::decodeSectorRecord()
{
    /* Skip ID mark. */

    readRawBits(16);

    const auto& rawbits = readRawBits(AESLANIER_RECORD_SIZE*16);
    const auto& bytes = decodeFmMfm(rawbits).slice(0, AESLANIER_RECORD_SIZE);
    const auto& reversed = reverse_bits(bytes);

    _sector->logicalTrack = reversed[1];
    _sector->logicalSide = 0;
    _sector->logicalSector = reversed[2];

    /* Check header 'checksum' (which seems far too simple to mean much). */

    {
        uint8_t wanted = reversed[3];
        uint8_t got = reversed[1] + reversed[2];
        if (wanted != got)
            return;
    }

    /* Check data checksum, which also includes the header and is
        * significantly better. */

    _sector->data = reversed.slice(1, AESLANIER_SECTOR_LENGTH);
    uint16_t wanted = reversed.reader().seek(0x101).read_le16();
    uint16_t got = crc16ref(MODBUS_POLY_REF, _sector->data);
    _sector->status = (wanted == got) ? Sector::OK : Sector::BAD_CHECKSUM;
}
