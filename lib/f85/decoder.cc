#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "f85.h"
#include "crc.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static Bytes decode(const std::vector<bool>& bits)
{
    Bytes output;
    ByteWriter bw(output);
    BitWriter bitw(bw);

    auto ii = bits.begin();
    while (ii != bits.end())
    {
        uint8_t inputfifo = 0;
        for (size_t i=0; i<5; i++)
        {
            if (ii == bits.end())
                break;
            inputfifo = (inputfifo<<1) | *ii++;
        }

        bitw.push(decode_data_gcr(inputfifo), 4);
    }
    bitw.flush();

    return output;
}

SectorVector DurangoF85Decoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    unsigned nextSector;
    unsigned nextTrack;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& bytes = decode(rawdata);

        if (bytes.size() < 4)
            continue;

        switch (bytes[0])
        {
            case 0xce: /* sector record */
            {
                headerIsValid = false;
                nextSector = bytes[3];
                nextTrack = bytes[1];

                uint16_t wantChecksum = bytes.reader().seek(5).read_be16();
                uint16_t gotChecksum = crc16(CCITT_POLY, 0xef21, bytes.slice(1, 4));
                headerIsValid = (wantChecksum == gotChecksum);
                break;
            }

            case 0xcb: /* data record */
            {
                if (!headerIsValid)
                    break;
                if (bytes.size() < (F85_SECTOR_LENGTH + 3))
                    continue;

                const auto& payload = bytes.slice(1, F85_SECTOR_LENGTH);
                uint16_t wantChecksum = bytes.reader().seek(F85_SECTOR_LENGTH+1).read_be16();
                uint16_t gotChecksum = crc16(CCITT_POLY, 0xbf84, payload);

                int status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
                auto sector = std::unique_ptr<Sector>(
					new Sector(status, nextTrack, 0, nextSector, payload));
                sectors.push_back(std::move(sector));
                break;
            }
        }
	}

	return sectors;
}

int DurangoF85Decoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffff;
    if (masked == F85_RECORD_SEPARATOR)
		return 6;
    return 0;
}
