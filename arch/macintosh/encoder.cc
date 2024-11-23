#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "macintosh.h"
#include "lib/core/crc.h"
#include "lib/data/image.h"
#include "fmt/format.h"
#include "lib/encoders/encoders.pb.h"
#include "lib/data/layout.h"
#include "arch/macintosh/macintosh.pb.h"
#include <ctype.h>

static bool lastBit;

static double clockRateUsForTrack(unsigned track)
{
    if (track < 16)
        return 2.63;
    if (track < 32)
        return 2.89;
    if (track < 48)
        return 3.20;
    if (track < 64)
        return 3.57;
    return 3.98;
}

static unsigned sectorsForTrack(unsigned track)
{
    if (track < 16)
        return 12;
    if (track < 32)
        return 11;
    if (track < 48)
        return 10;
    if (track < 64)
        return 9;
    return 8;
}

static int encode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
#define GCR_ENTRY(gcr, data) \
    case data:               \
        return gcr;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

/* This is extremely inspired by the MESS implementation, written by Nathan
 * Woods and R. Belmont:
 * https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap_dsk35.cpp
 */
static Bytes encode_crazy_data(const Bytes& input)
{
    Bytes output;
    ByteWriter bw(output);
    ByteReader br(input);

    uint8_t w1, w2, w3, w4;

    static const int LOOKUP_LEN = MAC_SECTOR_LENGTH / 3;

    uint8_t b1[LOOKUP_LEN + 1];
    uint8_t b2[LOOKUP_LEN + 1];
    uint8_t b3[LOOKUP_LEN + 1];

    uint32_t c1 = 0;
    uint32_t c2 = 0;
    uint32_t c3 = 0;
    for (int j = 0;; j++)
    {
        c1 = (c1 & 0xff) << 1;
        if (c1 & 0x0100)
            c1++;

        uint8_t val = br.read_8();
        c3 += val;
        if (c1 & 0x0100)
        {
            c3++;
            c1 &= 0xff;
        }
        b1[j] = (val ^ c1) & 0xff;

        val = br.read_8();
        c2 += val;
        if (c3 > 0xff)
        {
            c2++;
            c3 &= 0xff;
        }
        b2[j] = (val ^ c3) & 0xff;

        if (br.pos == 524)
            break;

        val = br.read_8();
        c1 += val;
        if (c2 > 0xff)
        {
            c1++;
            c2 &= 0xff;
        }
        b3[j] = (val ^ c2) & 0xff;
    }
    uint32_t c4 = ((c1 & 0xc0) >> 6) | ((c2 & 0xc0) >> 4) | ((c3 & 0xc0) >> 2);
    b3[LOOKUP_LEN] = 0;

    for (int i = 0; i <= LOOKUP_LEN; i++)
    {
        w1 = b1[i] & 0x3f;
        w2 = b2[i] & 0x3f;
        w3 = b3[i] & 0x3f;
        w4 = ((b1[i] & 0xc0) >> 2);
        w4 |= ((b2[i] & 0xc0) >> 4);
        w4 |= ((b3[i] & 0xc0) >> 6);

        bw.write_8(w4);
        bw.write_8(w1);
        bw.write_8(w2);

        if (i != LOOKUP_LEN)
            bw.write_8(w3);
    }

    bw.write_8(c4 & 0x3f);
    bw.write_8(c3 & 0x3f);
    bw.write_8(c2 & 0x3f);
    bw.write_8(c1 & 0x3f);

    return output;
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, const std::vector<bool>& src)
{
    for (bool bit : src)
    {
        if (cursor < bits.size())
            bits[cursor++] = bit;
    }
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
    cursor += width;
    for (int i = 0; i < width; i++)
    {
        unsigned pos = cursor - i - 1;
        if (pos < bits.size())
            bits[pos] = data & 1;
        data >>= 1;
    }
}

static uint8_t encode_side(uint8_t track, uint8_t side)
{
    /* Mac disks, being weird, use the side byte to encode both the side (in
     * bit 5) and also whether we're above track 0x3f (in bit 0).
     */

    return (side ? 0x20 : 0x00) | ((track > 0x3f) ? 0x01 : 0x00);
}

static void write_sector(std::vector<bool>& bits,
    unsigned& cursor,
    const std::shared_ptr<const Sector>& sector)
{
    if ((sector->data.size() != 512) && (sector->data.size() != 524))
        error("unsupported sector size --- you must pick 512 or 524");

    write_bits(bits, cursor, 0xff, 1 * 8); /* pad byte */
    for (int i = 0; i < 7; i++)
        write_bits(bits, cursor, 0xff3fcff3fcffLL, 6 * 8); /* sync */
    write_bits(bits, cursor, MAC_SECTOR_RECORD, 3 * 8);

    uint8_t encodedTrack = sector->logicalTrack & 0x3f;
    uint8_t encodedSector = sector->logicalSector;
    uint8_t encodedSide =
        encode_side(sector->logicalTrack, sector->logicalSide);
    uint8_t formatByte = MAC_FORMAT_BYTE;
    uint8_t headerChecksum =
        (encodedTrack ^ encodedSector ^ encodedSide ^ formatByte) & 0x3f;

    write_bits(bits, cursor, encode_data_gcr(encodedTrack), 1 * 8);
    write_bits(bits, cursor, encode_data_gcr(encodedSector), 1 * 8);
    write_bits(bits, cursor, encode_data_gcr(encodedSide), 1 * 8);
    write_bits(bits, cursor, encode_data_gcr(formatByte), 1 * 8);
    write_bits(bits, cursor, encode_data_gcr(headerChecksum), 1 * 8);

    write_bits(bits, cursor, 0xdeaaff, 3 * 8);
    write_bits(bits, cursor, 0xff3fcff3fcffLL, 6 * 8); /* sync */
    write_bits(bits, cursor, MAC_DATA_RECORD, 3 * 8);
    write_bits(bits, cursor, encode_data_gcr(sector->logicalSector), 1 * 8);

    Bytes wireData;
    wireData.writer()
        .append(sector->data.slice(512, 12))
        .append(sector->data.slice(0, 512));
    for (uint8_t b : encode_crazy_data(wireData))
        write_bits(bits, cursor, encode_data_gcr(b), 1 * 8);

    write_bits(bits, cursor, 0xdeaaff, 3 * 8);
}

class MacintoshEncoder : public Encoder
{
public:
    MacintoshEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.macintosh())
    {
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        double clockRateUs = clockRateUsForTrack(trackInfo->logicalTrack);
        int bitsPerRevolution = 200000.0 / clockRateUs;
        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        fillBitmapTo(bits,
            cursor,
            _config.post_index_gap_us() / clockRateUs,
            {true, false});
        lastBit = false;

        for (const auto& sector : sectors)
            write_sector(bits, cursor, sector);

        if (cursor >= bits.size())
            error("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(
            bits, calculatePhysicalClockPeriod(clockRateUs * 1e3, 200e6));
        return fluxmap;
    }

private:
    const MacintoshEncoderProto& _config;
};

std::unique_ptr<Encoder> createMacintoshEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new MacintoshEncoder(config));
}
