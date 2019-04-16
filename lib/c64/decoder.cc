#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "c64.h"
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

SectorVector Commodore64Decoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    unsigned nextSector;
    unsigned nextTrack;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& bytes = decode(rawdata);

        if (bytes.size() == 0)
            continue;

        switch (bytes[0])
        {
            case 8: /* sector record */
            {
                headerIsValid = false;
                if (bytes.size() < 6)
                    break;

                uint8_t checksum = bytes[1];
                nextSector = bytes[2];
                nextTrack = bytes[3] - 1;
                if (checksum != xorBytes(bytes.slice(2, 4)))
                    break;

                headerIsValid = true;
                break;
            }
            
            case 7: /* data record */
            {
                if (!headerIsValid)
                    break;
                headerIsValid = false;
                if (bytes.size() < 258)
                    break;

                Bytes payload = bytes.slice(1, C64_SECTOR_LENGTH);
                uint8_t gotChecksum = xorBytes(payload);
                uint8_t wantChecksum = bytes[257];
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

int Commodore64Decoder::recordMatcher(uint64_t fifo) const
{
    uint16_t masked = fifo & 0xffff;
    if (masked == C64_RECORD_SEPARATOR)
		return 4;
    return 0;
}
