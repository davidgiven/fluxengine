#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
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

static const FluxPattern SECTOR_START_PATTERN(16, 0xaaab);

nanoseconds_t ZilogMczDecoder::findSector(FluxmapReader& fmr, Track& track)
{
    fmr.seekToIndexMark();
    return fmr.seekToPattern(SECTOR_START_PATTERN);
}

void ZilogMczDecoder::decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector)
{
    fmr.readRawBits(14, sector.clock);

    auto rawbits = fmr.readRawBits(140*16, sector.clock);
    auto bytes = decodeFmMfm(rawbits).slice(0, 140);
    ByteReader br(bytes);

    sector.logicalSector = br.read_8() & 0x1f;
    sector.logicalSide = 0;
    sector.logicalTrack = br.read_8() & 0x7f;
    if (sector.logicalSector > 31)
        return;
    if (sector.logicalTrack > 80)
        return;

    sector.data = br.read(132);
    uint16_t wantChecksum = br.read_be16();
    uint16_t gotChecksum = crc16(MODBUS_POLY, 0x0000, bytes.slice(0, 134));

    sector.status = (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
