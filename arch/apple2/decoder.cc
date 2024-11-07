#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "protocol.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "apple2.h"
#include "arch/apple2/apple2.pb.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(24, APPLE2_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(24, APPLE2_DATA_RECORD);
const FluxMatchers ANY_RECORD_PATTERN(
    {&SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN});

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
#define GCR_ENTRY(gcr, data) \
    case gcr:                \
        return data;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

/* This is extremely inspired by the MESS implementation, written by Nathan
 * Woods and R. Belmont:
 * https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib/formats/ap2_dsk.cpp
 */
static Bytes decode_crazy_data(const uint8_t* inp, Sector::Status& status)
{
    Bytes output(APPLE2_SECTOR_LENGTH);

    uint8_t checksum = 0;
    for (unsigned i = 0; i < APPLE2_ENCODED_SECTOR_LENGTH; i++)
    {
        checksum ^= decode_data_gcr(*inp++);

        if (i >= 86)
        {
            /* 6 bit */
            output[i - 86] |= (checksum << 2);
        }
        else
        {
            /* 3 * 2 bit */
            output[i + 0] = ((checksum >> 1) & 0x01) | ((checksum << 1) & 0x02);
            output[i + 86] =
                ((checksum >> 3) & 0x01) | ((checksum >> 1) & 0x02);
            if ((i + 172) < APPLE2_SECTOR_LENGTH)
                output[i + 172] =
                    ((checksum >> 5) & 0x01) | ((checksum >> 3) & 0x02);
        }
    }

    checksum &= 0x3f;
    uint8_t wantedchecksum = decode_data_gcr(*inp);
    status = (checksum == wantedchecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    return output;
}

static uint8_t combine(uint16_t word)
{
    return word & (word >> 7);
}

class Apple2Decoder : public Decoder
{
public:
    Apple2Decoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        if (readRaw24() != APPLE2_SECTOR_RECORD)
            return;

        /* Read header. */

        auto header = toBytes(readRawBits(8 * 8)).slice(0, 8);
        ByteReader br(header);

        uint8_t volume = combine(br.read_be16());
        _sector->logicalTrack = combine(br.read_be16());
        _sector->logicalSide = _sector->physicalSide;
        _sector->logicalSector = combine(br.read_be16());
        uint8_t checksum = combine(br.read_be16());

        // If the checksum is correct, upgrade the sector from MISSING
        // to DATA_MISSING in anticipation of its data record
        if (checksum ==
            (volume ^ _sector->logicalTrack ^ _sector->logicalSector))
            _sector->status =
                Sector::DATA_MISSING; /* unintuitive but correct */

        if (_sector->logicalSide == 1)
            _sector->logicalTrack -= _config.apple2().side_one_track_offset();

        /* Sanity check. */

        if (_sector->logicalTrack > 100)
        {
            _sector->status = Sector::MISSING;
            return;
        }
    }

    void decodeDataRecord() override
    {
        /* Check ID. */

        if (readRaw24() != APPLE2_DATA_RECORD)
            return;

        // Sometimes there's a 1-bit gap between APPLE2_DATA_RECORD and
        // the data itself.  This has been seen on real world disks
        // such as the Apple II Operating System Kit from Apple2Online.
        // However, I haven't seen it described in any of the various
        // references.
        //
        // This extra '0' bit would not affect the real disk interface,
        // as it was a '1' reaching the top bit of a shift register
        // that triggered a byte to be available, but it affects the
        // way the data is read here.
        //
        // While the floppies tested only seemed to need this applied
        // to the first byte of the data record, applying it
        // consistently to all of them doesn't seem to hurt, and
        // simplifies the code.

        /* Read and decode data. */

        auto readApple8 = [&]()
        {
            auto result = 0;
            while ((result & 0x80) == 0)
            {
                auto b = readRawBits(1);
                if (b.empty())
                    break;
                result = (result << 1) | b[0];
            }
            return result;
        };

        constexpr unsigned recordLength = APPLE2_ENCODED_SECTOR_LENGTH + 2;
        uint8_t bytes[recordLength];
        for (auto& byte : bytes)
        {
            byte = readApple8();
        }

        // Upgrade the sector from MISSING to BAD_CHECKSUM.
        // If decode_crazy_data succeeds, it upgrades the sector to
        // OK.
        _sector->status = Sector::BAD_CHECKSUM;
        _sector->data = decode_crazy_data(&bytes[0], _sector->status);
    }
};

std::unique_ptr<Decoder> createApple2Decoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new Apple2Decoder(config));
}
