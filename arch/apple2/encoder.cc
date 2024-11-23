#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "arch/apple2/apple2.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "fmt/format.h"
#include "lib/encoders/encoders.pb.h"
#include <ctype.h>
#include "lib/core/bytes.h"

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

class Apple2Encoder : public Encoder
{
public:
    Apple2Encoder(const EncoderProto& config):
        Encoder(config),
        _config(config.apple2())
    {
    }

private:
    const Apple2EncoderProto& _config;

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        int bitsPerRevolution =
            (_config.rotational_period_ms() * 1e3) / _config.clock_period_us();

        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        for (const auto& sector : sectors)
            writeSector(bits, cursor, *sector);

        if (cursor >= bits.size())
            error("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), {true, false});

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits,
            calculatePhysicalClockPeriod(_config.clock_period_us() * 1e3,
                _config.rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    uint8_t volume_id = 254;

    /* This is extremely inspired by the MESS implementation, written by Nathan
     * Woods and R. Belmont:
     * https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib/formats/ap2_dsk.cpp
     * as well as Understanding the Apple II (1983) Chapter 9
     * https://archive.org/details/Understanding_the_Apple_II_1983_Quality_Software/page/n230/mode/1up?view=theater
     */

    void writeSector(
        std::vector<bool>& bits, unsigned& cursor, const Sector& sector) const
    {
        if ((sector.status == Sector::OK) or
            (sector.status == Sector::BAD_CHECKSUM))
        {
            auto write_bit = [&](bool val)
            {
                if (cursor <= bits.size())
                {
                    bits[cursor] = val;
                }
                cursor++;
            };

            auto write_bits = [&](uint32_t bits, int width)
            {
                for (int i = width; i--;)
                {
                    write_bit(bits & (1u << i));
                }
            };

            auto write_gcr44 = [&](uint8_t value)
            {
                write_bits((value << 7) | value | 0xaaaa, 16);
            };

            auto write_gcr6 = [&](uint8_t value)
            {
                write_bits(encode_data_gcr(value), 8);
            };

            // The special "FF40" sequence is used to synchronize the receiving
            // shift register. It's written as "1111 1111 00"; FF indicates the
            // 8 consecutive 1-bits, while "40" indicates the total number of
            // microseconds.
            auto write_ff40 = [&](int n = 1)
            {
                for (; n--;)
                {
                    write_bits(0xff << 2, 10);
                }
            };

            // There is data to encode to disk.
            if ((sector.data.size() != APPLE2_SECTOR_LENGTH))
                error("unsupported sector size {} --- you must pick 256",
                    sector.data.size());

            // Write address syncing leader : A sequence of "FF40"s; 5 of them
            // are said to suffice to synchronize the decoder.
            // "FF40" indicates that the actual data written is "1111
            // 1111 00" i.e., 8 1s and a total of 40 microseconds
            //
            // In standard formatting, the first logical sector apparently gets
            // extra padding.
            write_ff40(sector.logicalSector == 0 ? 32 : 8);

            int track = sector.logicalTrack;
            if (sector.logicalSide == 1)
                track += _config.side_one_track_offset();

            // Write address field: APPLE2_SECTOR_RECORD + sector identifier +
            // DE AA EB
            write_bits(APPLE2_SECTOR_RECORD, 24);
            write_gcr44(volume_id);
            write_gcr44(track);
            write_gcr44(sector.logicalSector);
            write_gcr44(volume_id ^ track ^ sector.logicalSector);
            write_bits(0xDEAAEB, 24);

            // Write data syncing leader: FF40 + APPLE2_DATA_RECORD + sector
            // data + sum + DE AA EB (+ mystery bits cut off of the scan?)
            write_ff40(8);
            write_bits(APPLE2_DATA_RECORD, 24);

            // Convert the sector data to GCR, append the checksum, and write it
            // out
            constexpr auto TWOBIT_COUNT =
                0x56; // Size of the 'twobit' area at the start of the GCR data
            uint8_t checksum = 0;
            for (int i = 0; i < APPLE2_ENCODED_SECTOR_LENGTH; i++)
            {
                int value;
                if (i >= TWOBIT_COUNT)
                {
                    value = sector.data[i - TWOBIT_COUNT] >> 2;
                }
                else
                {
                    uint8_t tmp = sector.data[i];
                    value = ((tmp & 1) << 1) | ((tmp & 2) >> 1);

                    tmp = sector.data[i + TWOBIT_COUNT];
                    value |= ((tmp & 1) << 3) | ((tmp & 2) << 1);

                    if (i + 2 * TWOBIT_COUNT < APPLE2_SECTOR_LENGTH)
                    {
                        tmp = sector.data[i + 2 * TWOBIT_COUNT];
                        value |= ((tmp & 1) << 5) | ((tmp & 2) << 3);
                    }
                }
                checksum ^= value;
                // assert(checksum & ~0x3f == 0);
                write_gcr6(checksum);
                checksum = value;
            }
            if (sector.status == Sector::BAD_CHECKSUM)
                checksum ^= 0x3f;
            write_gcr6(checksum);
            write_bits(0xDEAAEB, 24);
        }
    }
};

std::unique_ptr<Encoder> createApple2Encoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new Apple2Encoder(config));
}
