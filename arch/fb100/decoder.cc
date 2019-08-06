#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "record.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "fb100.h"
#include "crc.h"
#include "bytes.h"
#include "decoders/rawbits.h"
#include "track.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_ID_PATTERN(16, 0xabaa);

/* 
 * Reverse engineered from a dump of the floppy drive's ROM. I have no idea how
 * it works.
 * 
 * LF8BA:
 *         clra
 *         staa    X00B0
 *         staa    X00B1
 *         ldx     #$8000
 * LF8C2:  ldaa    $00,x
 *         inx
 *         bsr     LF8CF
 *         cpx     #$8011
 *         bne     LF8C2
 *         ldd     X00B0
 *         rts
 * LF8CF:
 *         eora    X00B0
 *         staa    X00CF
 *         asla
 *         asla
 *         asla
 *         asla
 *         eora    X00CF
 *         staa    X00CF
 *         rola
 *         rola
 *         rola
 *         tab
 *         anda    #$F8
 *         eora    X00B1
 *         staa    X00B0
 *         rolb
 *         rolb
 *         andb    #$0F
 *         eorb    X00B0
 *         stab    X00B0
 *         rolb
 *         eorb    X00CF
 *         stab    X00B1
 *         rts
 */

static void rol(uint8_t& b, bool& c)
{
    bool newc = b & 0x80;
    b <<= 1;
    b |= c;
    c = newc;
}

static uint16_t checksum(const Bytes& bytes)
{
    uint8_t crclo = 0;
    uint8_t crchi = 0;
    for (uint8_t a : bytes)
    {
        a ^= crchi;
        uint8_t t1 = a;
        a <<= 4;
        bool c = a & 0x10;
        a ^= t1;
        t1 = a;
        rol(a, c);
        rol(a, c);
        rol(a, c);
        uint8_t b = a;
        a &= 0xf8;
        a ^= crclo;
        crchi = a;
        rol(b, c);
        rol(b, c);
        b &= 0x0f;
        b ^= crchi;
        crchi = b;
        rol(b, c);
        b ^= t1;
        crclo = b;
    }

    return (crchi << 8) | crclo;
}

AbstractDecoder::RecordType Fb100Decoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(SECTOR_ID_PATTERN, matcher);
    if (matcher == &SECTOR_ID_PATTERN)
		return RecordType::SECTOR_RECORD;
	return RecordType::UNKNOWN_RECORD;
}

void Fb100Decoder::decodeSectorRecord()
{
    auto rawbits = readRawBits(FB100_RECORD_SIZE*16);

    const Bytes bytes = decodeFmMfm(rawbits).slice(0, FB100_RECORD_SIZE);
    ByteReader br(bytes);
    br.seek(1);
    const Bytes id = br.read(FB100_ID_SIZE);
    uint16_t wantIdCrc = br.read_be16();
    uint16_t gotIdCrc = checksum(id);
    const Bytes payload = br.read(FB100_PAYLOAD_SIZE);
    uint16_t wantPayloadCrc = br.read_be16();
    uint16_t gotPayloadCrc = checksum(payload);

    if (wantIdCrc != gotIdCrc)
        return;

    uint8_t abssector = id[2];
    _sector->logicalTrack = abssector >> 1;
    _sector->logicalSide = 0;
    _sector->logicalSector = abssector & 1;
    _sector->data.writer().append(id.slice(5, 12)).append(payload);

    _sector->status = (wantPayloadCrc == gotPayloadCrc) ? Sector::OK : Sector::BAD_CHECKSUM;
}