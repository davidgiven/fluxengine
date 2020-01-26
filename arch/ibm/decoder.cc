#include "globals.h"
#include "decoders/decoders.h"
#include "ibm.h"
#include "crc.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "sector.h"
#include "record.h"
#include <string.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value,
		"IbmIdam is not trivially copyable");

/*
 * The markers at the beginning of records are special, and have
 * missing clock pulses, allowing them to be found by the logic.
 * 
 * IAM record:
 * flux:   XXXX-XXX-XXXX-X- = 0xf77a
 * clock:  X X - X - X X X  = 0xd7
 * data:    X X X X X X - - = 0xfc
 * 
 * (We just ignore this one --- it's useless and optional.)
 */

/* 
 * IDAM record:
 * flux:   XXXX-X-X-XXXXXX- = 0xf57e
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X X X - = 0xfe
 */
const FluxPattern FM_IDAM_PATTERN(16, 0xf57e);

/* 
 * DAM1 record:
 * flux:   XXXX-X-X-XX-X-X- = 0xf56a
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - - - = 0xf8
 */
const FluxPattern FM_DAM1_PATTERN(16, 0xf56a);

/* 
 * DAM2 record:
 * flux:   XXXX-X-X-XX-XXXX = 0xf56f
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - X X = 0xfb
 */
const FluxPattern FM_DAM2_PATTERN(16, 0xf56f);

/* 
 * TRS80DAM1 record:
 * flux:   XXXX-X-X-XX-X-XX = 0xf56b
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - - X = 0xf9
 */
const FluxPattern FM_TRS80DAM1_PATTERN(16, 0xf56b);

/* 
 * TRS80DAM2 record:
 * flux:   XXXX-X-X-XX-XXX- = 0xf56c
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - X - = 0xfa
 */
const FluxPattern FM_TRS80DAM2_PATTERN(16, 0xf56c);

/* MFM record separator:
 * 0xA1 is:
 * data:    1  0  1  0  0  0  0  1  = 0xa1
 * mfm:     01 00 01 00 10 10 10 01 = 0x44a9
 * special: 01 00 01 00 10 00 10 01 = 0x4489
 *                       ^^^^^
 * When shifted out of phase, the special 0xa1 byte becomes an illegal
 * encoding (you can't do 10 00). So this can't be spoofed by user data.
 * 
 * shifted: 10 00 10 01 00 01 00 1
 * 
 * It's repeated three times.
 */
const FluxPattern MFM_PATTERN(48, 0x448944894489LL);

const FluxMatchers ANY_RECORD_PATTERN(
    {
        &MFM_PATTERN,
        &FM_IDAM_PATTERN,
        &FM_DAM1_PATTERN,
        &FM_DAM2_PATTERN,
        &FM_TRS80DAM1_PATTERN,
        &FM_TRS80DAM2_PATTERN,
    }
);

AbstractDecoder::RecordType IbmDecoder::advanceToNextRecord()
{
	const FluxMatcher* matcher = nullptr;
	_sector->clock = _fmr->seekToPattern(ANY_RECORD_PATTERN, matcher);

    /* If this is the MFM prefix byte, the the decoder is going to expect three
     * extra bytes on the front of the header. */
    _currentHeaderLength = (matcher == &MFM_PATTERN) ? 3 : 0;

    Fluxmap::Position here = tell();
    if (_currentHeaderLength > 0)
        readRawBits(_currentHeaderLength*16);
    auto idbits = readRawBits(16);
    const Bytes idbytes = decodeFmMfm(idbits);
    uint8_t id = idbytes.slice(0, 1)[0];
    seek(here);
    
    switch (id)
    {
        case IBM_IDAM:
            return RecordType::SECTOR_RECORD;

        case IBM_DAM1:
        case IBM_DAM2:
        case IBM_TRS80DAM1:
        case IBM_TRS80DAM2:
            return RecordType::DATA_RECORD;
    }
    return RecordType::UNKNOWN_RECORD;
}

void IbmDecoder::decodeSectorRecord()
{
    unsigned recordSize = _currentHeaderLength + IBM_IDAM_LEN;
    auto bits = readRawBits(recordSize*16);
    auto bytes = decodeFmMfm(bits).slice(0, recordSize);

    ByteReader br(bytes);
    br.seek(_currentHeaderLength);
    br.read_8(); /* skip ID byte */
    _sector->logicalTrack = br.read_8();
    _sector->logicalSide = br.read_8();
    _sector->logicalSector = br.read_8() - _sectorBase;
    _currentSectorSize = 1 << (br.read_8() + 7);
    uint16_t wantCrc = br.read_be16();
    uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, _currentHeaderLength + 5));
    if (wantCrc == gotCrc)
        _sector->status = Sector::DATA_MISSING; /* correct but unintuitive */

    if (_ignoreSideByte)
        _sector->logicalSide = _sector->physicalSide;
}

void IbmDecoder::decodeDataRecord()
{
    unsigned recordLength = _currentHeaderLength + _currentSectorSize + 3;
    auto bits = readRawBits(recordLength*16);
    auto bytes = decodeFmMfm(bits).slice(0, recordLength);

    ByteReader br(bytes);
    br.seek(_currentHeaderLength);
    br.read_8(); /* skip ID byte */

    _sector->data = br.read(_currentSectorSize);
    uint16_t wantCrc = br.read_be16();
    uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, recordLength-2));
    _sector->status = (wantCrc == gotCrc) ? Sector::OK : Sector::BAD_CHECKSUM;
}
