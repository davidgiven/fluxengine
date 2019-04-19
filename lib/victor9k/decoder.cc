#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "victor9k.h"
#include "crc.h"
#include "bytes.h"
#include "track.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(32, VICTOR9K_SECTOR_RECORD);
const FluxPatterns SECTOR_OR_DATA_RECORD_PATTERN(32, { VICTOR9K_SECTOR_RECORD, VICTOR9K_DATA_RECORD });

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

        uint8_t decoded = decode_data_gcr(inputfifo);
        bitw.push(decoded, 4);
    }
    bitw.flush();

    return output;
}

nanoseconds_t Victor9kDecoder::findSector(FluxmapReader& fmr, Track& track)
{
    return fmr.seekToPattern(SECTOR_RECORD_PATTERN);
}

nanoseconds_t Victor9kDecoder::findData(FluxmapReader& fmr, Track& track)
{
    return fmr.seekToPattern(SECTOR_OR_DATA_RECORD_PATTERN);
}

void Victor9kDecoder::decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector)
{
    /* Skip the sync marker bit. */
    fmr.readRawBits(23, sector.clock);

    /* Read header. */

    auto bytes = decode(fmr.readRawBits(4*10, sector.clock)).slice(0, 4);

    uint8_t rawTrack = bytes[1];
    sector.logicalSector = bytes[2];
    uint8_t gotChecksum = bytes[3];

    sector.logicalTrack = rawTrack & 0x7f;
    sector.logicalSide = rawTrack >> 7;
    uint8_t wantChecksum = bytes[1] + bytes[2];
    if ((sector.logicalSector > 20) || (sector.logicalTrack > 85) || (sector.logicalSide > 1))
        return;
                
    if (wantChecksum == gotChecksum)
        sector.status = Sector::DATA_MISSING; /* unintuitive but correct */
}

void Victor9kDecoder::decodeData(FluxmapReader& fmr, Track& track, Sector& sector)
{
    /* Skip the sync marker bit. */
    fmr.readRawBits(23, sector.clock);

    /* Read data. */

    auto bytes = decode(fmr.readRawBits((VICTOR9K_SECTOR_LENGTH+5)*10, sector.clock))
        .slice(0, VICTOR9K_SECTOR_LENGTH+5);
    ByteReader br(bytes);

    /* Check that this is actually a data record. */
    
    if (br.read_8() != 8)
        return;

    sector.data = br.read(VICTOR9K_SECTOR_LENGTH);
    uint16_t gotChecksum = sumBytes(sector.data);
    uint16_t wantChecksum = br.read_le16();
    sector.status = (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
