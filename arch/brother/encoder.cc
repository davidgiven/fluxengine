#include "globals.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "brother.h"
#include "crc.h"
#include "writer.h"
#include "image.h"
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
        Error() << "unsupported sector size";

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

static int charToInt(char c)
{
    if (isdigit(c))
        return c - '0';
    return 10 + tolower(c) - 'a';
}

class BrotherEncoder : public AbstractEncoder
{
public:
    BrotherEncoder(const EncoderProto& config):
        AbstractEncoder(config),
        _config(config.brother())
    {
    }

public:
    std::vector<std::shared_ptr<const Sector>> collectSectors(
		const Location& location,
        const Image& image) override
    {
        std::vector<std::shared_ptr<const Sector>> sectors;

        if (location.head != 0)
            return sectors;

        switch (_config.format())
        {
            case BROTHER120:
                if (location.logicalTrack >= BROTHER_TRACKS_PER_120KB_DISK)
                    return sectors;
                break;

            case BROTHER240:
                if (location.logicalTrack >= BROTHER_TRACKS_PER_240KB_DISK)
                    return sectors;
                break;
        }

        const std::string& skew = _config.sector_skew();
        for (int sectorCount = 0; sectorCount < BROTHER_SECTORS_PER_TRACK;
             sectorCount++)
        {
            int sectorId = charToInt(skew.at(sectorCount));
            const auto& sector = image.get(location.logicalTrack, 0, sectorId);
            if (sector)
                sectors.push_back(sector);
        }

        return sectors;
    }

    std::unique_ptr<Fluxmap> encode(
		const Location& location,
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
            write_sector_header(
                bits, cursor, sectorData->logicalTrack, sectorData->logicalSector);
            fillBitmapTo(bits, cursor, dataCursor, {true, false});
            write_sector_data(bits, cursor, sectorData->data);

			sectorCount++;
        }

        if (cursor >= bits.size())
            Error() << "track data overrun";
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits, _config.clock_rate_us() * 1e3);
        return fluxmap;
    }

private:
    const BrotherEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createBrotherEncoder(
    const EncoderProto& config)
{
    return std::unique_ptr<AbstractEncoder>(new BrotherEncoder(config));
}
