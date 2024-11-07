#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "arch/tartu/tartu.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include <string.h>

class TartuEncoder : public Encoder
{
public:
    TartuEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.tartu())
    {
    }

    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        _clockRateUs = _config.clock_period_us();
        int bitsPerRevolution =
            (_config.target_rotational_period_ms() * 1000.0) / _clockRateUs;

        const auto& sector = *sectors.begin();
        _bits.resize(bitsPerRevolution);
        _cursor = 0;

        writeFillerRawBitsUs(_config.gap1_us());
        bool first = true;
        for (const auto& sectorData : sectors)
        {
            if (!first)
                writeFillerRawBitsUs(_config.gap4_us());
            first = false;
            writeSector(sectorData);
        }

        if (_cursor > _bits.size())
            error("track data overrun");
        writeFillerRawBitsUs(_config.target_rotational_period_ms() * 1000.0);

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(_bits,
            calculatePhysicalClockPeriod(_clockRateUs * 1e3,
                _config.target_rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    void writeBytes(const Bytes& bytes)
    {
        encodeMfm(_bits, _cursor, bytes, _lastBit);
    }

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

    void writeFillerRawBitsUs(double us)
    {
        unsigned count = (us / _clockRateUs) / 2;
        for (int i = 0; i < count; i++)
            writeRawBits(0b10, 2);
    };

    void writeSector(const std::shared_ptr<const Sector>& sectorData)
    {
        writeRawBits(_config.header_marker(), 64);
        {
            Bytes bytes;
            ByteWriter bw(bytes);
            bw.write_8(
                (sectorData->logicalTrack << 1) | sectorData->logicalSide);
            bw.write_8(1);
            bw.write_8(sectorData->logicalSector);
            bw.write_8(~sumBytes(bytes.slice(0, 3)));
            writeBytes(bytes);
        }

        writeFillerRawBitsUs(_config.gap3_us());
        writeRawBits(_config.data_marker(), 64);
        {
            Bytes bytes;
            ByteWriter bw(bytes);
            bw.append(sectorData->data);
            bw.write_8(~sumBytes(bytes.slice(0, sectorData->data.size())));
            writeBytes(bytes);
        }
    }

private:
    const TartuEncoderProto& _config;
    double _clockRateUs;
    std::vector<bool> _bits;
    unsigned _cursor;
    bool _lastBit;
};

std::unique_ptr<Encoder> createTartuEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new TartuEncoder(config));
}
