#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "fb100.h"
#include "crc.h"
#include "bytes.h"
#include "rawbits.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

static bool search(const RawBits& rawbits, size_t& cursor)
{
    uint16_t fifo = 0;

    while (cursor < rawbits.size())
    {
        fifo = (fifo << 1) | rawbits[cursor++];
        
        if (fifo == 0xabaa)
        {
            cursor -= 16;
            return true;
        }
    }

    return false;
}

void Fb100Decoder::decodeToSectors(const RawBits& rawbits, unsigned,
    RawRecordVector& rawrecords, SectorVector& sectors)
{
    size_t cursor = 0;

    for (;;)
    {
        if (!search(rawbits, cursor))
            break;

        unsigned record_start = cursor;
        cursor = std::min(cursor + FB100_RECORD_SIZE*16, rawbits.size());
        std::vector<bool> recordbits(rawbits.begin() + record_start, rawbits.begin() + cursor);

        rawrecords.push_back(
            std::unique_ptr<RawRecord>(
                new RawRecord(
                    record_start,
                    recordbits.begin(),
                    recordbits.end())
            )
        );

        const Bytes bytes = decodeFmMfm(recordbits).slice(0, FB100_RECORD_SIZE);
        ByteReader br(bytes);
        br.seek(1);
        const Bytes id = br.read(FB100_ID_SIZE);
        uint16_t wantIdCrc = br.read_be16();
        const Bytes payload = br.read(FB100_PAYLOAD_SIZE);
        uint16_t wantPayloadCrc = br.read_be16();

        uint8_t abssector = id[2];
        uint8_t track = abssector >> 1;
        uint8_t sectorid = abssector & 1;

        int status = Sector::BAD_CHECKSUM;
        auto sector = std::unique_ptr<Sector>(
            new Sector(status, track, 0, sectorid, payload));
        sectors.push_back(std::move(sector));
    }
}