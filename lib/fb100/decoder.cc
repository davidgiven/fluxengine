#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "fb100.h"
#include "crc.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

SectorVector Fb100Decoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    unsigned nextSector;
    unsigned nextTrack;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const Bytes bytes = decodeFmMfm(rawrecord->data);
        hexdump(std::cout, bytes);
        if (bytes.size() < 0x515)
            continue;

        uint8_t abssector = bytes[2];
        uint8_t track = abssector >> 1;
        uint8_t sectorid = abssector & 1;

        uint16_t wantHeaderCrc = bytes.reader().seek(0x11).read_be16();

        const Bytes payload = bytes.slice(0x13, FB100_SECTOR_SIZE);
        uint16_t wantPayloadCrc = bytes.reader().seek(0x513).read_be16();
        // hexdumpForSrp16(std::cout, payload);
        // std::cout << fmt::format("{:04x}\n", wantPayloadCrc);

        int status = Sector::OK;
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, 0, sectorid, payload));
        sectors.push_back(std::move(sector));
    }

    return sectors;
}

int Fb100Decoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo;
    if (masked == 0xeaaaaeea)
        return 22;
    return 0;
}

