#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "protocol.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "macintosh.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(24, MAC_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(24, MAC_DATA_RECORD);
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

/* This is extremely inspired by the MESS implementation, written by Nathan
 * Woods and R. Belmont:
 * https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap_dsk35.cpp
 */
static Bytes decode_crazy_data(const Bytes& input, Sector::Status& status)
{
    Bytes output;
    ByteWriter bw(output);
    ByteReader br(input);

    static const int LOOKUP_LEN = MAC_SECTOR_LENGTH / 3;

    uint8_t b1[LOOKUP_LEN + 1];
    uint8_t b2[LOOKUP_LEN + 1];
    uint8_t b3[LOOKUP_LEN + 1];

    for (int i = 0; i <= LOOKUP_LEN; i++)
    {
        uint8_t w4 = br.read_8();
        uint8_t w1 = br.read_8();
        uint8_t w2 = br.read_8();
        uint8_t w3 = (i != 174) ? br.read_8() : 0;

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
        bw.write_8(val);

        val = b2[count] ^ c3;
        c2 += val;
        if (c3 > 0xFF)
        {
            c2++;
            c3 &= 0xFF;
        }
        bw.write_8(val);

        if (output.size() == 524)
            break;

        val = b3[count] ^ c2;
        c1 += val;
        if (c2 > 0xFF)
        {
            c1++;
            c2 &= 0xFF;
        }
        bw.write_8(val);
        count++;
    }

    uint8_t c4 = ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
    c1 &= 0x3f;
    c2 &= 0x3f;
    c3 &= 0x3f;
    c4 &= 0x3f;
    uint8_t g4 = br.read_8();
    uint8_t g3 = br.read_8();
    uint8_t g2 = br.read_8();
    uint8_t g1 = br.read_8();
    if ((g4 == c4) && (g3 == c3) && (g2 == c2) && (g1 == c1))
        status = Sector::OK;

    return output;
}

uint8_t decode_side(uint8_t side)
{
    /* Mac disks, being weird, use the side byte to encode both the side (in
     * bit 5) and also whether we're above track 0x3f (in bit 0).
     */

    return !!(side & 0x20);
}

class MacintoshDecoder : public Decoder
{
public:
    MacintoshDecoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        if (readRaw24() != MAC_SECTOR_RECORD)
            return;

        /* Read header. */

        auto header = toBytes(readRawBits(7 * 8)).slice(0, 7);

        uint8_t encodedTrack = decode_data_gcr(header[0]);
        if (encodedTrack != (_sector->physicalTrack & 0x3f))
            return;

        uint8_t encodedSector = decode_data_gcr(header[1]);
        uint8_t encodedSide = decode_data_gcr(header[2]);
        uint8_t formatByte = decode_data_gcr(header[3]);
        uint8_t wantedsum = decode_data_gcr(header[4]);

        if (encodedSector > 11)
            return;

        _sector->logicalTrack = _sector->physicalTrack;
        _sector->logicalSide = decode_side(encodedSide);
        _sector->logicalSector = encodedSector;
        uint8_t gotsum =
            (encodedTrack ^ encodedSector ^ encodedSide ^ formatByte) & 0x3f;
        if (wantedsum == gotsum)
            _sector->status =
                Sector::DATA_MISSING; /* unintuitive but correct */
    }

    void decodeDataRecord() override
    {
        if (readRaw24() != MAC_DATA_RECORD)
            return;

        /* Read data. */

        readRawBits(8); /* skip spare byte */
        auto inputbuffer = toBytes(readRawBits(MAC_ENCODED_SECTOR_LENGTH * 8))
                               .slice(0, MAC_ENCODED_SECTOR_LENGTH);

        for (unsigned i = 0; i < inputbuffer.size(); i++)
            inputbuffer[i] = decode_data_gcr(inputbuffer[i]);

        _sector->status = Sector::BAD_CHECKSUM;
        Bytes userData = decode_crazy_data(inputbuffer, _sector->status);
        _sector->data.clear();
        _sector->data.writer()
            .append(userData.slice(12, 512))
            .append(userData.slice(0, 12));
    }
};

std::unique_ptr<Decoder> createMacintoshDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new MacintoshDecoder(config));
}
