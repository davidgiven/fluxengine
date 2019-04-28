#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
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

const FluxPattern RECORD_SEPARATOR_PATTERN(16, C64_RECORD_SEPARATOR);

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

nanoseconds_t Commodore64Decoder::findSector(FluxmapReader& fmr, Track& track)
{
    nanoseconds_t clock = fmr.seekToPattern(RECORD_SEPARATOR_PATTERN);
    if (clock)
        fmr.readRawBits(12, clock);
    return clock;
}

nanoseconds_t Commodore64Decoder::findData(FluxmapReader& fmr, Track& track)
{
    return findSector(fmr, track);
}

void Commodore64Decoder::decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector)
{
    const auto& idbits = fmr.readRawBits(1*10, sector.clock);
    const auto& idbytes = decode(idbits).slice(0, 1);
    if (idbytes[0] != 8)
        return; /* Not a sector record */

    const auto& bits = fmr.readRawBits(5*10, sector.clock);
    const auto& bytes = decode(bits).slice(0, 5);

    uint8_t checksum = bytes[0];
    sector.logicalSector = bytes[1];
    sector.logicalSide = 0;
    sector.logicalTrack = bytes[2] - 1;
    if (checksum == xorBytes(bytes.slice(1, 4)))
        sector.status = Sector::DATA_MISSING; /* unintuitive but correct */
}

void Commodore64Decoder::decodeData(FluxmapReader& fmr, Track& track, Sector& sector)
{
    const auto& idbits = fmr.readRawBits(1*10, sector.clock);
    const auto& idbytes = decode(idbits).slice(0, 1);
    if (idbytes[0] != 7)
        return; /* Not a data record */

    const auto& bits = fmr.readRawBits(259*10, sector.clock);
    const auto& bytes = decode(bits).slice(0, 259);

    sector.data = bytes.slice(0, C64_SECTOR_LENGTH);
    uint8_t gotChecksum = xorBytes(sector.data);
    uint8_t wantChecksum = bytes[256];
    sector.status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
