#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
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

const FluxPattern SECTOR_RECORD_PATTERN(24, F85_SECTOR_RECORD);
const FluxPatterns SECTOR_OR_DATA_RECORD_PATTERN(24, { F85_SECTOR_RECORD, F85_DATA_RECORD });

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

nanoseconds_t DurangoF85Decoder::findSector(FluxmapReader& fmr, Track& track)
{
    return fmr.seekToPattern(SECTOR_RECORD_PATTERN);
}

nanoseconds_t DurangoF85Decoder::findData(FluxmapReader& fmr, Track& track)
{
    return fmr.seekToPattern(SECTOR_OR_DATA_RECORD_PATTERN);
}

void DurangoF85Decoder::decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector)
{
    /* Skip sync bits and ID byte. */

    fmr.readRawBits(24, sector.clock);

    /* Read header. */

    const auto& bytes = decode(fmr.readRawBits(6*10, sector.clock));

    sector.logicalSector = bytes[2];
    sector.logicalSide = 0;
    sector.logicalTrack = bytes[0];

    uint16_t wantChecksum = bytes.reader().seek(4).read_be16();
    uint16_t gotChecksum = crc16(CCITT_POLY, 0xef21, bytes.slice(0, 4));
    if (wantChecksum == gotChecksum)
        sector.status = Sector::DATA_MISSING; /* unintuitive but correct */
}

void DurangoF85Decoder::decodeData(FluxmapReader& fmr, Track& track, Sector& sector)
{
    /* Skip sync bits and check ID byte. */

    fmr.readRawBits(14, sector.clock);
    uint8_t idbyte = decode(fmr.readRawBits(10, sector.clock)).slice(0, 1)[0];
    if (idbyte != 0xcb)
        return;

    const auto& bytes = decode(fmr.readRawBits((F85_SECTOR_LENGTH+3)*10, sector.clock))
        .slice(0, F85_SECTOR_LENGTH+3);
    ByteReader br(bytes);

    sector.data = br.read(F85_SECTOR_LENGTH);
    uint16_t wantChecksum = br.read_be16();
    uint16_t gotChecksum = crc16(CCITT_POLY, 0xbf84, sector.data);
    sector.status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
