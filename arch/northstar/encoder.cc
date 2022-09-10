#include "globals.h"
#include "northstar.h"
#include "sector.h"
#include "bytes.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "image.h"
#include "mapper.h"
#include "lib/encoders/encoders.pb.h"

#define GAP_FILL_SIZE_SD 30
#define PRE_HEADER_GAP_FILL_SIZE_SD 9
#define GAP_FILL_SIZE_DD 62
#define PRE_HEADER_GAP_FILL_SIZE_DD 16

#define GAP1_FILL_BYTE (0x4F)
#define GAP2_FILL_BYTE (0x4F)

#define TOTAL_SECTOR_BYTES ()

static void write_sector(std::vector<bool>& bits,
    unsigned& cursor,
    const std::shared_ptr<const Sector>& sector)
{
    int preambleSize = 0;
    int encodedSectorSize = 0;
    int gapFillSize = 0;
    int preHeaderGapFillSize = 0;

    bool doubleDensity;

    switch (sector->data.size())
    {
        case NORTHSTAR_PAYLOAD_SIZE_SD:
            preambleSize = NORTHSTAR_PREAMBLE_SIZE_SD;
            encodedSectorSize = PRE_HEADER_GAP_FILL_SIZE_SD +
                                NORTHSTAR_ENCODED_SECTOR_SIZE_SD +
                                GAP_FILL_SIZE_SD;
            gapFillSize = GAP_FILL_SIZE_SD;
            preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_SD;
            doubleDensity = false;
            break;
        case NORTHSTAR_PAYLOAD_SIZE_DD:
            preambleSize = NORTHSTAR_PREAMBLE_SIZE_DD;
            encodedSectorSize = PRE_HEADER_GAP_FILL_SIZE_DD +
                                NORTHSTAR_ENCODED_SECTOR_SIZE_DD +
                                GAP_FILL_SIZE_DD;
            gapFillSize = GAP_FILL_SIZE_DD;
            preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_DD;
            doubleDensity = true;
            break;
        default:
            Error() << "unsupported sector size --- you must pick 256 or 512";
            break;
    }

    int fullSectorSize = preambleSize + encodedSectorSize;
    auto fullSector = std::make_shared<std::vector<uint8_t>>();
    fullSector->reserve(fullSectorSize);

    /* sector gap after index pulse */
    for (int i = 0; i < preHeaderGapFillSize; i++)
        fullSector->push_back(GAP1_FILL_BYTE);

    /* sector preamble */
    for (int i = 0; i < preambleSize; i++)
        fullSector->push_back(0);

    Bytes sectorData;
    if (sector->data.size() == encodedSectorSize)
        sectorData = sector->data;
    else
    {
        ByteWriter writer(sectorData);
        writer.write_8(0xFB); /* sync character */
        if (doubleDensity == true)
        {
            writer.write_8(0xFB); /* Double-density has two sync characters */
        }
        writer += sector->data;
        if (doubleDensity == true)
        {
            writer.write_8(northstarChecksum(sectorData.slice(2)));
        }
        else
        {
            writer.write_8(northstarChecksum(sectorData.slice(1)));
        }
    }
    for (uint8_t b : sectorData)
        fullSector->push_back(b);

    if (sector->physicalSector != 9)
    {
        /* sector postamble */
        for (int i = 0; i < gapFillSize; i++)
            fullSector->push_back(GAP2_FILL_BYTE);

        if (fullSector->size() != fullSectorSize)
            Error() << "sector mismatched length (" << sector->data.size()
                    << ") expected: " << fullSector->size() << " got "
                    << fullSectorSize;
    }
    else
    {
        /* sector postamble */
        for (int i = 0; i < gapFillSize; i++)
            fullSector->push_back(GAP2_FILL_BYTE);
    }

    bool lastBit = false;

    if (doubleDensity == true)
    {
        encodeMfm(bits, cursor, fullSector, lastBit);
    }
    else
    {
        encodeFm(bits, cursor, fullSector);
    }
}

class NorthstarEncoder : public AbstractEncoder
{
public:
    NorthstarEncoder(const EncoderProto& config):
        AbstractEncoder(config),
        _config(config.northstar())
    {
    }

    std::vector<std::shared_ptr<const Sector>> collectSectors(
        const Location& location, const Image& image) override
    {
        std::vector<std::shared_ptr<const Sector>> sectors;

        if ((location.logicalTrack >= 0) && (location.logicalTrack < 35))
        {
            for (int sectorId = 0; sectorId < 10; sectorId++)
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
        int bitsPerRevolution = 100000;
        double clockRateUs = _config.clock_period_us();

        const auto& sector = *sectors.begin();
        if (sector->data.size() == NORTHSTAR_PAYLOAD_SIZE_SD)
            bitsPerRevolution /= 2; // FM
        else
            clockRateUs /= 2.00;

        std::vector<bool> bits(bitsPerRevolution);
        unsigned cursor = 0;

        for (const auto& sectorData : sectors)
            write_sector(bits, cursor, sectorData);

        if (cursor > bits.size())
            Error() << "track data overrun";

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits,
            Mapper::calculatePhysicalClockPeriod(
                clockRateUs * 1e3, _config.rotational_period_ms() * 1e6));
        return fluxmap;
    }

private:
    const NorthstarEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createNorthstarEncoder(
    const EncoderProto& config)
{
    return std::unique_ptr<AbstractEncoder>(new NorthstarEncoder(config));
}
