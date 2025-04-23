#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "aeslanier5.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/data/sector.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>

static const FluxPattern SECTOR_PATTERN(32, AESLANIER5_RECORD_SEPARATOR);

/* This is actually FM, rather than MFM, but it our MFM/FM decoder copes fine
 * with it. */

class AesLanier5Decoder : public Decoder
{
public:
    AesLanier5Decoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(SECTOR_PATTERN);
    }

    void decodeSectorRecord() override
    {
        /* Skip ID mark (we know it's a AESLANIER5_RECORD_SEPARATOR). */

        readRawBits(SECTOR_PATTERN.length());

        const auto& rawbits = readRawBits(AESLANIER5_RECORD_SIZE * 16);
        const auto& bytes =
            decodeFmMfm(rawbits).slice(0, AESLANIER5_RECORD_SIZE);
        const auto& reversed = bytes.reverseBits();

        uint8_t encodedTrack = reversed[0];
        uint8_t encodedSector = reversed[1];

        _sector->logicalTrack = encodedTrack >> 1;
        _sector->logicalSide = 0;
        _sector->logicalSector = encodedSector;

        /* Check header 'checksum' (which seems far too simple to mean much). */

        {
            uint8_t wanted = reversed[2];
            uint8_t got = reversed[0] + reversed[1];
            if (wanted != got)
                return;
        }

        /* Check data checksum, which also includes the header and is
         * significantly better. */

        _sector->data = reversed.slice(3, AESLANIER5_SECTOR_LENGTH);
        uint8_t wanted, got;
        ByteReader br(_sector->data);
        if ((encodedSector == 0) || (encodedSector == 8))
        {
            wanted = br.seek(17).read_8() + br.seek(150).read_8();
            got = sumBytes(_sector->data.slice(0, 17)) + sumBytes(_sector->data.slice(18, 132));
        }
        else
        {
            wanted = br.seek(150).read_8();
            got = sumBytes(_sector->data.slice(0, AESLANIER5_SECTOR_LENGTH-1));
        }
        _sector->status = (wanted == got) ? Sector::OK : Sector::BAD_CHECKSUM;
    }
};

std::unique_ptr<Decoder> createAesLanier5Decoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new AesLanier5Decoder(config));
}
