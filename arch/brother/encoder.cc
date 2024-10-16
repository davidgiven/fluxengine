#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "brother.h"
#include "lib/core/crc.h"
#include "lib/data/image.h"
#include "arch/brother/brother.pb.h"
#include "lib/encoders/encoders.pb.h"

static int encode_header_gcr(uint16_t word)
{
    switch (word)
    {
#define GCR_ENTRY(gcr, data) \
    case data:               \
        return gcr;
#include "header_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static int encode_data_gcr(uint8_t data)
{
    switch (data)
    {
#define GCR_ENTRY(gcr, data) \
    case data:               \
        return gcr;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static void write_bits(
    std::vector<bool>& bits, unsigned& cursor, uint32_t data, int width)
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

static void write_sector_header(
    std::vector<bool>& bits, unsigned& cursor, int track, int sector)
{
    write_bits(bits, cursor, 0xffffffff, 31);
    write_bits(bits, cursor, BROTHER_SECTOR_RECORD, 32);
    write_bits(bits, cursor, encode_header_gcr(track), 16);
    write_bits(bits, cursor, encode_header_gcr(sector), 16);
    write_bits(bits, cursor, encode_header_gcr(0x2f), 16);
}

static void write_sector_data(
    std::vector<bool>& bits, unsigned& cursor, const Bytes& data)
{
    write_bits(bits, cursor, 0xffffffff, 32);
    write_bits(bits, cursor, BROTHER_DATA_RECORD, 32);

    uint16_t fifo = 0;
    int width = 0;

    if (data.size() != BROTHER_DATA_RECORD_PAYLOAD)
        error("unsupported sector size");

    auto write_byte = [&](uint8_t byte)
    {
        fifo |= (byte << (8 - width));
        width += 8;

        while (width >= 5)
        {
            uint8_t quintet = fifo >> 11;
            fifo <<= 5;
            width -= 5;

            write_bits(bits, cursor, encode_data_gcr(quintet), 8);
        }
    };

    for (uint8_t byte : data)
        write_byte(byte);

    uint32_t realCrc = crcbrother(data);
    write_byte(realCrc >> 16);
    write_byte(realCrc >> 8);
    write_byte(realCrc);
    write_byte(0x58); /* magic */
    write_byte(0xd4);
    while (width != 0)
        write_byte(0);
}

class BrotherEncoder : public Encoder
{
public:
    BrotherEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.brother())
    {
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        int bitsPerRevolution = 200000.0 / _config.clock_rate_us();
        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        int sectorCount = 0;
        for (const auto& sectorData : sectors)
        {
            double headerMs = _config.post_index_gap_ms() +
                              sectorCount * _config.sector_spacing_ms();
            unsigned headerCursor = headerMs * 1e3 / _config.clock_rate_us();
            double dataMs = headerMs + _config.post_header_spacing_ms();
            unsigned dataCursor = dataMs * 1e3 / _config.clock_rate_us();

            fillBitmapTo(bits, cursor, headerCursor, {true, false});
            write_sector_header(bits,
                cursor,
                sectorData->logicalTrack,
                sectorData->logicalSector);
            fillBitmapTo(bits, cursor, dataCursor, {true, false});
            write_sector_data(bits, cursor, sectorData->data);

            sectorCount++;
        }

        if (cursor >= bits.size())
            error("track data overrun");
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits, _config.clock_rate_us() * 1e3);
        return fluxmap;
    }

private:
    const BrotherEncoderProto& _config;
};

std::unique_ptr<Encoder> createBrotherEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new BrotherEncoder(config));
}
