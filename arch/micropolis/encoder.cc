#include "lib/core/globals.h"
#include "micropolis.h"
#include "lib/data/sector.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/data/image.h"
#include "lib/encoders/encoders.pb.h"

static void write_sector(std::vector<bool>& bits,
    unsigned& cursor,
    const std::shared_ptr<const Sector>& sector,
    MicropolisEncoderProto::EccType eccType)
{
    if ((sector->data.size() != 256) &&
        (sector->data.size() != MICROPOLIS_ENCODED_SECTOR_SIZE))
        error("unsupported sector size --- you must pick 256 or 275");

    int fullSectorSize = 40 + MICROPOLIS_ENCODED_SECTOR_SIZE + 40 + 35;
    auto fullSector = std::make_shared<std::vector<uint8_t>>();
    fullSector->reserve(fullSectorSize);
    /* sector preamble */
    for (int i = 0; i < 40; i++)
        fullSector->push_back(0);
    Bytes sectorData;
    if (sector->data.size() == MICROPOLIS_ENCODED_SECTOR_SIZE)
    {
        if (sector->data[0] != 0xFF)
            error(
                "275 byte sector doesn't start with sync byte 0xFF. "
                "Corrupted sector");
        uint8_t wantChecksum = sector->data[1 + 2 + 266];
        uint8_t gotChecksum =
            micropolisChecksum(sector->data.slice(1, 2 + 266));
        if (wantChecksum != gotChecksum)
            std::cerr << "Warning: checksum incorrect. Sector: "
                      << sector->logicalSector << std::endl;
        sectorData = sector->data;
    }
    else
    {
        ByteWriter writer(sectorData);
        writer.write_8(0xff); /* Sync */
        writer.write_8(sector->logicalTrack);
        writer.write_8(sector->logicalSector);
        for (int i = 0; i < 10; i++)
            writer.write_8(0); /* Padding */
        writer += sector->data;
        writer.write_8(micropolisChecksum(sectorData.slice(1)));

        uint8_t eccPresent = 0;
        uint32_t ecc = 0;
        if (eccType == MicropolisEncoderProto::VECTOR)
        {
            eccPresent = 0xaa;
            ecc = vectorGraphicEcc(sectorData + Bytes(4));
        }
        writer.write_be32(ecc);
        writer.write_8(eccPresent);
    }
    for (uint8_t b : sectorData)
        fullSector->push_back(b);
    /* sector postamble */
    for (int i = 0; i < 40; i++)
        fullSector->push_back(0);
    /* filler */
    for (int i = 0; i < 35; i++)
        fullSector->push_back(0);

    if (fullSector->size() != fullSectorSize)
        error("sector mismatched length");
    bool lastBit = false;
    encodeMfm(bits, cursor, fullSector, lastBit);
    /* filler */
    for (int i = 0; i < 5; i++)
    {
        bits[cursor++] = 1;
        bits[cursor++] = 0;
    }
}

class MicropolisEncoder : public Encoder
{
public:
    MicropolisEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.micropolis())
    {
    }

    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        int bitsPerRevolution =
            (_config.rotational_period_ms() * 1e3) / _config.clock_period_us();

        std::vector<bool> bits(bitsPerRevolution);
        std::vector<unsigned> indexes;
        unsigned prev_cursor = 0;
        unsigned cursor = 0;

        for (const auto& sectorData : sectors)
        {
            indexes.push_back(cursor);
            prev_cursor = cursor;
            write_sector(bits, cursor, sectorData, _config.ecc_type());
        }
        indexes.push_back(prev_cursor + (cursor - prev_cursor) / 2);
        indexes.push_back(cursor);

        if (cursor != bits.size())
            error("track data mismatched length");

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        nanoseconds_t clockPeriod =
            calculatePhysicalClockPeriod(_config.clock_period_us() * 1e3,
                _config.rotational_period_ms() * 1e6);
        auto pos = bits.begin();
        for (int i = 1; i < indexes.size(); i++)
        {
            auto end = bits.begin() + indexes[i];
            fluxmap->appendBits(std::vector<bool>(pos, end), clockPeriod);
            fluxmap->appendIndex();
            pos = end;
        }
        return fluxmap;
    }

private:
    const MicropolisEncoderProto& _config;
};

std::unique_ptr<Encoder> createMicropolisEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new MicropolisEncoder(config));
}
