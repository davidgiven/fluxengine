#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "tids990.h"
#include "lib/core/crc.h"
#include "lib/data/image.h"
#include "arch/tids990/tids990.pb.h"
#include "lib/encoders/encoders.pb.h"
#include "fmt/format.h"

static int charToInt(char c)
{
    if (isdigit(c))
        return c - '0';
    return 10 + tolower(c) - 'a';
}

static uint8_t decodeUint16(uint16_t raw)
{
    Bytes b;
    ByteWriter bw(b);
    bw.write_be16(raw);
    return decodeFmMfm(b.toBits())[0];
}

class Tids990Encoder : public Encoder
{
public:
    Tids990Encoder(const EncoderProto& config):
        Encoder(config),
        _config(config.tids990())
    {
    }

private:
    void writeRawBits(uint32_t data, int width)
    {
        _cursor += width;
        _lastBit = data & 1;
        for (int i = 0; i < width; i++)
        {
            unsigned pos = _cursor - i - 1;
            if (pos < _bits.size())
                _bits[pos] = data & 1;
            data >>= 1;
        }
    }

    void writeBytes(const Bytes& bytes)
    {
        encodeMfm(_bits, _cursor, bytes, _lastBit);
    }

    void writeBytes(int count, uint8_t byte)
    {
        Bytes bytes = {byte};
        for (int i = 0; i < count; i++)
            writeBytes(bytes);
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        double clockRateUs = _config.clock_period_us() / 2.0;
        int bitsPerRevolution =
            (_config.rotational_period_ms() * 1000.0) / clockRateUs;
        _bits.resize(bitsPerRevolution);
        _cursor = 0;

        uint8_t am1Unencoded = decodeUint16(_config.am1_byte());
        uint8_t am2Unencoded = decodeUint16(_config.am2_byte());

        writeBytes(_config.gap1_bytes(), 0x55);

        bool first = true;
        for (const auto& sectorData : sectors)
        {
            if (!first)
                writeBytes(_config.gap3_bytes(), 0x55);
            first = false;

            /* Writing the sector and data records are fantastically annoying.
             * The CRC is calculated from the *very start* of the record, and
             * include the malformed marker bytes. Our encoder doesn't know
             * about this, of course, with the result that we have to construct
             * the unencoded header, calculate the checksum, and then use the
             * same logic to emit the bytes which require special encoding
             * before encoding the rest of the header normally. */

            {
                Bytes header;
                ByteWriter bw(header);

                writeBytes(12, 0x55);
                bw.write_8(am1Unencoded);
                bw.write_8(sectorData->logicalSide << 3);
                bw.write_8(sectorData->logicalTrack);
                bw.write_8(_config.sector_count());
                bw.write_8(sectorData->logicalSector);
                bw.write_be16(sectorData->data.size());
                uint16_t crc = crc16(CCITT_POLY, header);
                bw.write_be16(crc);

                writeRawBits(_config.am1_byte(), 16);
                writeBytes(header.slice(1));
            }

            writeBytes(_config.gap2_bytes(), 0x55);

            {
                Bytes data;
                ByteWriter bw(data);

                writeBytes(12, 0x55);
                bw.write_8(am2Unencoded);

                bw += sectorData->data;
                uint16_t crc = crc16(CCITT_POLY, data);
                bw.write_be16(crc);

                writeRawBits(_config.am2_byte(), 16);
                writeBytes(data.slice(1));
            }
        }

        if (_cursor >= _bits.size())
            error("track data overrun");
        while (_cursor < _bits.size())
            writeBytes(1, 0x55);

        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBits(_bits,
            calculatePhysicalClockPeriod(
                clockRateUs * 1e3, _config.rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    const Tids990EncoderProto& _config;
    std::vector<bool> _bits;
    unsigned _cursor;
    bool _lastBit;
};

std::unique_ptr<Encoder> createTids990Encoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new Tids990Encoder(config));
}
