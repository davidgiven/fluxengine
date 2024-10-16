#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "victor9k.h"
#include "lib/core/crc.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "fmt/format.h"
#include "arch/victor9k/victor9k.pb.h"
#include "lib/encoders/encoders.pb.h"
#include "lib/data/layout.h"
#include <ctype.h>
#include "lib/core/bytes.h"

static bool lastBit;

static void write_zero_bits(
    std::vector<bool>& bits, unsigned& cursor, unsigned count)
{
    while (count--)
    {
        if (cursor < bits.size())
            lastBit = bits[cursor++] = 0;
    }
}

static void write_one_bits(
    std::vector<bool>& bits, unsigned& cursor, unsigned count)
{
    while (count--)
    {
        if (cursor < bits.size())
            lastBit = bits[cursor++] = 1;
    }
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, const std::vector<bool>& src)
{
    for (bool bit : src)
    {
        if (cursor < bits.size())
            lastBit = bits[cursor++] = bit;
    }
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
    cursor += width;
    lastBit = data & 1;
    for (int i = 0; i < width; i++)
    {
        unsigned pos = cursor - i - 1;
        if (pos < bits.size())
            bits[pos] = data & 1;
        data >>= 1;
    }
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, const Bytes& bytes)
{
    ByteReader br(bytes);
    BitReader bitr(br);

    while (!bitr.eof())
    {
        if (cursor < bits.size())
            bits[cursor++] = bitr.get();
    }
}

static int encode_data_gcr(uint8_t data)
{
    switch (data & 0x0f)
    {
#define GCR_ENTRY(gcr, data) \
    case data:               \
        return gcr;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static void write_byte(std::vector<bool>& bits, unsigned& cursor, uint8_t b)
{
    write_bits(bits, cursor, encode_data_gcr(b >> 4), 5);
    write_bits(bits, cursor, encode_data_gcr(b), 5);
}

static void write_bytes(
    std::vector<bool>& bits, unsigned& cursor, const Bytes& bytes)
{
    for (uint8_t b : bytes)
        write_byte(bits, cursor, b);
}

static void write_gap(std::vector<bool>& bits, unsigned& cursor, int length)
{
    for (int i = 0; i < length / 10; i++)
        write_byte(bits, cursor, '0');
}

static void write_sector(std::vector<bool>& bits,
    unsigned& cursor,
    const Victor9kEncoderProto::TrackdataProto& trackdata,
    const Sector& sector)
{
    write_one_bits(bits, cursor, trackdata.pre_header_sync_bits());
    write_bits(bits, cursor, VICTOR9K_SECTOR_RECORD, 10);

    uint8_t encodedTrack = sector.logicalTrack | (sector.logicalSide << 7);
    uint8_t encodedSector = sector.logicalSector;
    write_bytes(bits,
        cursor,
        Bytes{
            encodedTrack,
            encodedSector,
            (uint8_t)(encodedTrack + encodedSector),
        });

    write_gap(bits, cursor, trackdata.post_header_gap_bits());

    write_one_bits(bits, cursor, trackdata.pre_data_sync_bits());
    write_bits(bits, cursor, VICTOR9K_DATA_RECORD, 10);

    write_bytes(bits, cursor, sector.data);

    Bytes checksum(2);
    checksum.writer().write_le16(sumBytes(sector.data));
    write_bytes(bits, cursor, checksum);
    write_gap(bits, cursor, trackdata.post_data_gap_bits());
}

class Victor9kEncoder : public Encoder
{
public:
    Victor9kEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.victor9k())
    {
    }

private:
    void getTrackFormat(Victor9kEncoderProto::TrackdataProto& trackdata,
        unsigned track,
        unsigned head)
    {
        trackdata.Clear();
        for (const auto& f : _config.trackdata())
        {
            if (f.has_min_track() && (track < f.min_track()))
                continue;
            if (f.has_max_track() && (track > f.max_track()))
                continue;
            if (f.has_head() && (head != f.head()))
                continue;

            trackdata.MergeFrom(f);
        }
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        Victor9kEncoderProto::TrackdataProto trackdata;
        getTrackFormat(
            trackdata, trackInfo->logicalTrack, trackInfo->logicalSide);

        unsigned bitsPerRevolution = (trackdata.rotational_period_ms() * 1e3) /
                                     trackdata.clock_period_us();
        std::vector<bool> bits(bitsPerRevolution);
        nanoseconds_t clockPeriod =
            calculatePhysicalClockPeriod(trackdata.clock_period_us() * 1e3,
                trackdata.rotational_period_ms() * 1e6);
        unsigned cursor = 0;

        fillBitmapTo(bits,
            cursor,
            trackdata.post_index_gap_us() * 1e3 / clockPeriod,
            {true, false});
        lastBit = false;

        for (const auto& sector : sectors)
            write_sector(bits, cursor, trackdata, *sector);

        if (cursor >= bits.size())
            error("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits, clockPeriod);
        return fluxmap;
    }

private:
    const Victor9kEncoderProto& _config;
};

std::unique_ptr<Encoder> createVictor9kEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new Victor9kEncoder(config));
}

// vim: sw=4 ts=4 et
