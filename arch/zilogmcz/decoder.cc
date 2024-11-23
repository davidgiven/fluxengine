#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "protocol.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "zilogmcz.h"
#include "lib/core/bytes.h"
#include "lib/core/crc.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

static const FluxPattern SECTOR_START_PATTERN(16, 0xaaab);

class ZilogMczDecoder : public Decoder
{
public:
    ZilogMczDecoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        seekToIndexMark();
        return seekToPattern(SECTOR_START_PATTERN);
    }

    void decodeSectorRecord() override
    {
        readRawBits(14);

        auto rawbits = readRawBits(140 * 16);
        auto bytes = decodeFmMfm(rawbits).slice(0, 140);
        ByteReader br(bytes);

        _sector->logicalSector = br.read_8() & 0x1f;
        _sector->logicalSide = 0;
        _sector->logicalTrack = br.read_8() & 0x7f;
        if (_sector->logicalSector > 31)
            return;
        if (_sector->logicalTrack > 80)
            return;

        _sector->data = br.read(132);
        uint16_t wantChecksum = br.read_be16();
        uint16_t gotChecksum = crc16(MODBUS_POLY, 0x0000, bytes.slice(0, 134));

        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }
};

std::unique_ptr<Decoder> createZilogMczDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new ZilogMczDecoder(config));
}
