#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "zilogmcz.h"
#include "bytes.h"
#include "crc.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

static std::vector<bool>::const_iterator find_start_of_data(const std::vector<bool>& rawbits)
{
    uint8_t fifo = 0;
    auto ii = rawbits.begin();

    while (ii != rawbits.end())
    {
        fifo = (fifo << 1) | *ii++;
        if (fifo == 0xab)
            return ii-2;
    }

    return ii;
}

SectorVector ZilogMczDecoder::decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack, unsigned physicalSide)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        auto start = find_start_of_data(rawrecord->data);
        auto rawbytes = decodeFmMfm(start, rawrecord->data.cend()).slice(0, 136);

        uint8_t sectorid = rawbytes[0] & 0x1f;
        uint8_t track = rawbytes[1] & 0x7f;
        if (sectorid > 31)
            continue;
        if (track > 80)
            continue;

        Bytes payload = rawbytes.slice(2, 132);
        uint16_t wantChecksum = rawbytes.reader().seek(134).read_be16();
        uint16_t gotChecksum = crc16(MODBUS_POLY, 0x0000, rawbytes.slice(0, 134));

        int status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, 0, sectorid, payload));
        sectors.push_back(std::move(sector));
	}

	return sectors;
}
