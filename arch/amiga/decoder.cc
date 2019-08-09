#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "amiga.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

/* 
 * Amiga disks use MFM but it's not quite the same as IBM MFM. They only use
 * a single type of record with a different marker byte.
 * 
 * See the big comment in the IBM MFM decoder for the gruesome details of how
 * MFM works.
 */
         
static const FluxPattern SECTOR_PATTERN(48, AMIGA_SECTOR_RECORD);

static Bytes deinterleave(const uint8_t*& input, size_t len)
{
    assert(!(len & 1));
    const uint8_t* odds = &input[0];
    const uint8_t* evens = &input[len/2];
    Bytes output;
    ByteWriter bw(output);

    for (size_t i=0; i<len/2; i++)
    {
        uint8_t o = *odds++;
        uint8_t e = *evens++;

        /* This is the 'Interleave bits with 64-bit multiply' technique from
         * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
         */
        uint16_t result =
            (((e * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 49) & 0x5555) |
            (((o * 0x0101010101010101ULL & 0x8040201008040201ULL)
                * 0x0102040810204081ULL >> 48) & 0xAAAA);
        
        bw.write_be16(result);
    }

    input += len;
    return output;
}

static uint32_t checksum(const Bytes& bytes)
{
    ByteReader br(bytes);
    uint32_t checksum = 0;

    assert((bytes.size() & 3) == 0);
    while (!br.eof())
        checksum ^= br.read_be32();

    return checksum & 0x55555555;
}

AbstractDecoder::RecordType AmigaDecoder::advanceToNextRecord()
{
    _sector->clock = _fmr->seekToPattern(SECTOR_PATTERN);
    if (_fmr->eof() || !_sector->clock)
        return UNKNOWN_RECORD;
    return SECTOR_RECORD;
}

void AmigaDecoder::decodeSectorRecord()
{
    const auto& rawbits = readRawBits(AMIGA_RECORD_SIZE*16);
    const auto& rawbytes = toBytes(rawbits).slice(0, AMIGA_RECORD_SIZE*2);
    const auto& bytes = decodeFmMfm(rawbits).slice(0, AMIGA_RECORD_SIZE);

    const uint8_t* ptr = bytes.begin() + 3;

    Bytes header = deinterleave(ptr, 4);
    Bytes recoveryinfo = deinterleave(ptr, 16);

    _sector->logicalTrack = header[1] >> 1;
    _sector->logicalSide = header[1] & 1;
    _sector->logicalSector = header[2];

    uint32_t wantedheaderchecksum = deinterleave(ptr, 4).reader().read_be32();
    uint32_t gotheaderchecksum = checksum(rawbytes.slice(6, 40));
    if (gotheaderchecksum != wantedheaderchecksum)
        return;

    uint32_t wanteddatachecksum = deinterleave(ptr, 4).reader().read_be32();
    uint32_t gotdatachecksum = checksum(rawbytes.slice(62, 1024));

    _sector->data.clear();
    _sector->data.writer().append(deinterleave(ptr, 512)).append(recoveryinfo);
    _sector->status = (gotdatachecksum == wanteddatachecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
}
