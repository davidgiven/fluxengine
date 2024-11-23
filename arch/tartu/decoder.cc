#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/decoders/decoders.h"
#include "arch/tartu/tartu.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/data/sector.h"
#include <string.h>

constexpr uint64_t HEADER_BITS = 0xaaaaaaaa44895554LL;
constexpr uint64_t DATA_BITS = 0xaaaaaaaa44895545LL;

static const FluxPattern HEADER_PATTERN(64, HEADER_BITS);
static const FluxPattern DATA_PATTERN(64, DATA_BITS);

const FluxMatchers ANY_RECORD_PATTERN{&HEADER_PATTERN, &DATA_PATTERN};

class TartuDecoder : public Decoder
{
public:
    TartuDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.tartu())
    {
    }

    void beginTrack() override {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        if (readRaw64() != HEADER_BITS)
            return;

        auto bits = readRawBits(16 * 4);
        auto bytes = decodeFmMfm(bits).slice(0, 4);

        ByteReader br(bytes);
        uint8_t track = br.read_8();
        _sector->logicalTrack = track >> 1;
        _sector->logicalSide = track & 1;
        br.skip(1); /* seems always to be 1 */
        _sector->logicalSector = br.read_8();
        uint8_t wantChecksum = br.read_8();
        uint8_t gotChecksum = ~sumBytes(bytes.slice(0, 3));

        if (wantChecksum == gotChecksum)
            _sector->status = Sector::DATA_MISSING;

        _sector->status = Sector::DATA_MISSING;
    }

    void decodeDataRecord() override
    {
        if (readRaw64() != DATA_BITS)
            return;

        const auto& bits = readRawBits(129 * 16);
        const auto& bytes = decodeFmMfm(bits).slice(0, 129);
        _sector->data = bytes.slice(0, 128);

        uint8_t wantChecksum = bytes.reader().seek(128).read_8();
        uint8_t gotChecksum = ~sumBytes(_sector->data);
        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }

private:
    const TartuDecoderProto& _config;
};

std::unique_ptr<Decoder> createTartuDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new TartuDecoder(config));
}
