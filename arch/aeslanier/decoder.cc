#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "aeslanier.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/data/sector.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>

static const FluxPattern SECTOR_PATTERN(32, AESLANIER_RECORD_SEPARATOR);

/* This is actually M2FM, rather than MFM, but it our MFM/FM decoder copes fine
 * with it. */

class AesLanierDecoder : public Decoder
{
public:
    AesLanierDecoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(SECTOR_PATTERN);
    }

    void decodeSectorRecord() override
    {
        /* Skip ID mark (we know it's a AESLANIER_RECORD_SEPARATOR). */

        readRawBits(16);

        const auto& rawbits = readRawBits(AESLANIER_RECORD_SIZE * 16);
        const auto& bytes =
            decodeFmMfm(rawbits).slice(0, AESLANIER_RECORD_SIZE);
        const auto& reversed = bytes.reverseBits();

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
};

std::unique_ptr<Decoder> createAesLanierDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new AesLanierDecoder(config));
}
