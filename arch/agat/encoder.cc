#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "agat.h"
#include "lib/core/crc.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "arch/agat/agat.pb.h"
#include "lib/encoders/encoders.pb.h"

class AgatEncoder : public Encoder
{
public:
    AgatEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.agat())
    {
    }

private:
    void writeRawBits(uint64_t data, int width)
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

    void writeByte(uint8_t byte)
    {
        Bytes b;
        b.writer().write_8(byte);
        writeBytes(b);
    }

    void writeFillerRawBytes(int count, uint16_t byte)
    {
        for (int i = 0; i < count; i++)
            writeRawBits(byte, 16);
    };

    void writeFillerBytes(int count, uint8_t byte)
    {
        Bytes b{byte};
        for (int i = 0; i < count; i++)
            writeBytes(b);
    };

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        auto trackLayout = Layout::getLayoutOfTrack(
            trackInfo->logicalTrack, trackInfo->logicalSide);

        double clockRateUs = _config.target_clock_period_us() / 2.0;
        int bitsPerRevolution =
            (_config.target_rotational_period_ms() * 1000.0) / clockRateUs;
        _bits.resize(bitsPerRevolution);
        _cursor = 0;

        writeFillerRawBytes(_config.post_index_gap_bytes(), 0xaaaa);

        for (const auto& sector : sectors)
        {
            /* Header */

            writeFillerRawBytes(_config.pre_sector_gap_bytes(), 0xaaaa);
            writeRawBits(SECTOR_ID, 64);
            writeByte(0x5a);
            writeByte((sector->logicalTrack << 1) | sector->logicalSide);
            writeByte(sector->logicalSector);
            writeByte(0x5a);

            /* Data */

            writeFillerRawBytes(_config.pre_data_gap_bytes(), 0xaaaa);
            auto data = sector->data.slice(0, AGAT_SECTOR_SIZE);
            writeRawBits(DATA_ID, 64);
            writeBytes(data);
            writeByte(agatChecksum(data));
            writeByte(0x5a);
        }

        if (_cursor >= _bits.size())
            error("track data overrun");
        fillBitmapTo(_bits, _cursor, _bits.size(), {true, false});

        auto fluxmap = std::make_unique<Fluxmap>();
        fluxmap->appendBits(_bits,
            calculatePhysicalClockPeriod(_config.target_clock_period_us() * 1e3,
                _config.target_rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    const AgatEncoderProto& _config;
    uint32_t _cursor;
    bool _lastBit;
    std::vector<bool> _bits;
};

std::unique_ptr<Encoder> createAgatEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new AgatEncoder(config));
}
