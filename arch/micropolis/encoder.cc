#include "globals.h"
#include "micropolis.h"
#include "sector.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "image.h"
#include "lib/encoders/encoders.pb.h"

static void write_sector(std::vector<bool>& bits,
    unsigned& cursor,
    const std::shared_ptr<const Sector>& sector)
{
    if ((sector->data.size() != 256) &&
        (sector->data.size() != MICROPOLIS_ENCODED_SECTOR_SIZE))
        Error() << "unsupported sector size --- you must pick 256 or 275";

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
            Error() << "275 byte sector doesn't start with sync byte 0xFF. "
                       "Corrupted sector";
        uint8_t wantChecksum = sector->data[1 + 2 + 266];
        uint8_t gotChecksum =
            micropolisChecksum(sector->data.slice(1, 2 + 266));
        if (wantChecksum != gotChecksum)
            std::cerr << "Warning: checksum incorrect. Sector: "
                      << sector->physicalSector << std::endl;
        sectorData = sector->data;
    }
    else
    {
        ByteWriter writer(sectorData);
        writer.write_8(0xff); /* Sync */
        writer.write_8(sector->logicalTrack);
        writer.write_8(sector->physicalSector);
        for (int i = 0; i < 10; i++)
            writer.write_8(0); /* Padding */
        writer += sector->data;
        writer.write_8(micropolisChecksum(sectorData.slice(1)));
        for (int i = 0; i < 5; i++)
            writer.write_8(0); /* 4 byte ECC and ECC not present flag */
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
        Error() << "sector mismatched length";
    bool lastBit = false;
    encodeMfm(bits, cursor, fullSector, lastBit);
    /* filler */
    for (int i = 0; i < 5; i++)
    {
        bits[cursor++] = 1;
        bits[cursor++] = 0;
    }
}

class MicropolisEncoder : public AbstractEncoder
{
public:
    MicropolisEncoder(const EncoderProto& config):
        AbstractEncoder(config),
        _config(config.micropolis())
    {
    }

    std::vector<std::shared_ptr<const Sector>> collectSectors(
        const Location& location, const Image& image) override
    {
        std::vector<std::shared_ptr<const Sector>> sectors;

        if ((location.logicalTrack >= 0) && (location.logicalTrack < 77))
        {
            for (int sectorId = 0; sectorId < 16; sectorId++)
            {
                const auto& sector =
                    image.get(location.logicalTrack, location.head, sectorId);
                if (sector)
                    sectors.push_back(sector);
            }
        }

        return sectors;
    }

    std::unique_ptr<Fluxmap> encode(const Location& location,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        int bitsPerRevolution =
            (_config.rotational_period_ms() * 1e3) / _config.clock_period_us();

        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        for (const auto& sectorData : sectors)
            write_sector(bits, cursor, sectorData);

        if (cursor != bits.size())
            Error() << "track data mismatched length";

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits,
            calculatePhysicalClockPeriod(
                _config.clock_period_us() * 1e3,
                _config.rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    const MicropolisEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createMicropolisEncoder(
    const EncoderProto& config)
{
    return std::unique_ptr<AbstractEncoder>(new MicropolisEncoder(config));
}
