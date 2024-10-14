#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "protocol.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "victor9k.h"
#include "lib/core/crc.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(32, VICTOR9K_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(32, VICTOR9K_DATA_RECORD);
const FluxMatchers ANY_RECORD_PATTERN(
    {&SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN});

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
#define GCR_ENTRY(gcr, data) \
    case gcr:                \
        return data;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static Bytes decode(const std::vector<bool>& bits)
{
    Bytes output;
    ByteWriter bw(output);
    BitWriter bitw(bw);

    auto ii = bits.begin();
    while (ii != bits.end())
    {
        uint8_t inputfifo = 0;
        for (size_t i = 0; i < 5; i++)
        {
            if (ii == bits.end())
                break;
            inputfifo = (inputfifo << 1) | *ii++;
        }

        uint8_t decoded = decode_data_gcr(inputfifo);
        bitw.push(decoded, 4);
    }
    bitw.flush();

    return output;
}

class Victor9kDecoder : public Decoder
{
public:
    Victor9kDecoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        /* Check the ID. */

        if (readRaw32() != VICTOR9K_SECTOR_RECORD)
            return;

        /* Read header. */

        auto bytes = decode(readRawBits(3 * 10)).slice(0, 3);

        uint8_t rawTrack = bytes[0];
        _sector->logicalSector = bytes[1];
        uint8_t gotChecksum = bytes[2];

        _sector->logicalTrack = rawTrack & 0x7f;
        _sector->logicalSide = rawTrack >> 7;
        uint8_t wantChecksum = bytes[0] + bytes[1];
        if ((_sector->logicalSector > 20) || (_sector->logicalTrack > 85) ||
            (_sector->logicalSide > 1))
            return;

        if (wantChecksum == gotChecksum)
            _sector->status =
                Sector::DATA_MISSING; /* unintuitive but correct */
    }

    void decodeDataRecord() override
    {
        /* Check the ID. */

        if (readRaw32() != VICTOR9K_DATA_RECORD)
            return;

        /* Read data. */

        auto bytes = decode(readRawBits((VICTOR9K_SECTOR_LENGTH + 4) * 10))
                         .slice(0, VICTOR9K_SECTOR_LENGTH + 4);
        ByteReader br(bytes);

        _sector->data = br.read(VICTOR9K_SECTOR_LENGTH);
        uint16_t gotChecksum = sumBytes(_sector->data);
        uint16_t wantChecksum = br.read_le16();
        _sector->status =
            (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }
};

std::unique_ptr<Decoder> createVictor9kDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new Victor9kDecoder(config));
}
