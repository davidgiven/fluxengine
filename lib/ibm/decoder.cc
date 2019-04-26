#include "globals.h"
#include "decoders.h"
#include "ibm.h"
#include "crc.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
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
 */
const FluxPattern MFM_PATTERN(16, 0x4489);

const FluxMatchers SEPARATOR_PATTERNS(
    {
        &MFM_PATTERN,
        &FM_IDAM_PATTERN,
        // &FM_DAM1_PATTERN,
        &FM_DAM2_PATTERN,
        // &FM_TRS80DAM1_PATTERN,
        // &FM_TRS80DAM2_PATTERN,
    }
);

nanoseconds_t IbmDecoder::findSector(FluxmapReader& fmr, Track& track)
{
    for (;;)
    {
        nanoseconds_t clock = fmr.seekToPattern(SEPARATOR_PATTERNS);
        if (fmr.eof() || !clock)
            return 0;

        auto here = fmr.tell();
        auto idbits = fmr.readRawBits(16, clock);
        uint8_t id = decodeFmMfm(idbits).slice(0, 1)[0];
        fmr.seek(here);
        /* Check for the MFM prefix, and skip it. */
        if (id == IBM_MFM_SYNC)
            fmr.readRawBits(48, clock);
        switch (id)
        {
            case IBM_IDAM:
            case IBM_DAM1:
            case IBM_DAM2:
            case IBM_TRS80DAM1:
            case IBM_TRS80DAM2:
                return clock;
        }

        fmr.readRawBit(clock);
    }
}

nanoseconds_t IbmDecoder::findData(FluxmapReader& fmr, Track& track)
{
    return findSector(fmr, track);
}

void IbmDecoder::decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector)
{
    auto bits = fmr.readRawBits(IBM_IDAM_LEN*16, sector.clock);
    auto bytes = decodeFmMfm(bits).slice(0, IBM_IDAM_LEN);

    ByteReader br(bytes);
    uint8_t id = br.read_8();
    if (id != IBM_IDAM)
        return;
    sector.logicalTrack = br.read_8();
    sector.logicalSide = br.read_8();
    sector.logicalSector = br.read_8() - _sectorBase;
    _sectorSize = 1 << (br.read_8() + 7);
    uint16_t wantCrc = br.read_be16();
    uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, 5));
    if (wantCrc == gotCrc)
        sector.status = Sector::DATA_MISSING; /* correct but unintuitive */
}

void IbmDecoder::decodeData(FluxmapReader& fmr, Track& track, Sector& sector)
{
    unsigned recordLength = _sectorSize + 3;
    auto bits = fmr.readRawBits(recordLength*16, sector.clock);
    auto bytes = decodeFmMfm(bits).slice(0, recordLength);

    ByteReader br(bytes);
    uint8_t id = br.read_8();
    if ((id != IBM_DAM1) && (id != IBM_DAM2) && (id != IBM_TRS80DAM1) && (id != IBM_TRS80DAM2))
        return;

    sector.data = br.read(_sectorSize);
    uint16_t wantCrc = br.read_be16();
    uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, recordLength-2));
    sector.status = (wantCrc == gotCrc) ? Sector::OK : Sector::BAD_CHECKSUM;
}

#if 0
SectorVector AbstractIbmDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned, unsigned)
{
    bool idamValid = false;
    IbmIdam idam;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const Bytes bytes = decodeFmMfm(rawrecord->data);
        unsigned headerSize = skipHeaderBytes();
        if (bytes.size() < (headerSize + 1))
            continue;
        Bytes data = bytes.slice(headerSize, bytes.size() - headerSize);

        switch (data[0])
        {
            case IBM_IAM:
                /* Track header. Ignore. */
                break;

            case IBM_IDAM:
            {
                if (data.size() < sizeof(idam))
                    break;
                memcpy(&idam, data.cbegin(), sizeof(idam));

				uint16_t crc = crc16(CCITT_POLY, bytes.slice(0, offsetof(IbmIdam, crc) + headerSize));
				uint16_t wantedCrc = (idam.crc[0]<<8) | idam.crc[1];
				idamValid = (crc == wantedCrc);
                break;
            }

            case IBM_DAM1:
            case IBM_DAM2:
            case IBM_TRS80DAM1:
            case IBM_TRS80DAM2:
            {
                if (!idamValid)
                    break;
                
                unsigned size = 1 << (idam.sectorSize + 7);
                data.resize(IBM_DAM_LEN + size + 2);

				uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, IBM_DAM_LEN+size+headerSize));
				uint16_t wantedCrc = data.reader().seek(IBM_DAM_LEN+size).read_be16();
				int status = (gotCrc == wantedCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                int sectorNum = idam.sector - _sectorIdBase;
                auto sector = std::unique_ptr<Sector>(
					new Sector(status, idam.cylinder, idam.side, sectorNum,
                        data.slice(IBM_DAM_LEN, size)));
                sectors.push_back(std::move(sector));
                idamValid = false;
                break;
            }
        }
    }

    return sectors;
}

nanoseconds_t IbmMfmDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock() / 2;
}

int IbmFmDecoder::recordMatcher(uint64_t fifo) const
{
    /*
     * The markers at the beginning of records are special, and have
     * missing clock pulses, allowing them to be found by the logic.
     * 
     * IAM record:
     * flux:   XXXX-XXX-XXXX-X- = 0xf77a
     * clock:  X X - X - X X X  = 0xd7
     * data:    X X X X X X - - = 0xfc
     * 
     * IDAM record:
     * flux:   XXXX-X-X-XXXXXX- = 0xf57e
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X X X - = 0xfe
     * 
     * DAM1 record:
     * flux:   XXXX-X-X-XX-X-X- = 0xf56a
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - - - = 0xf8
     * 
     * DAM2 record:
     * flux:   XXXX-X-X-XX-XXXX = 0xf56f
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - X X = 0xfb
     * 
     * TRS80DAM1 record:
     * flux:   XXXX-X-X-XX-X-XX = 0xf56b
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - - X = 0xf9
     * 
     * TRS80DAM2 record:
     * flux:   XXXX-X-X-XX-XXX- = 0xf56c
     * clock:  X X - - - X X X  = 0xc7
     * data:    X X X X X - X - = 0xfa
     */
         
    uint16_t masked = fifo & 0xffff;
    switch (masked)
    {
        case 0xf77a:
        case 0xf57e:
        case 0xf56a:
        case 0xf56f:
        case 0xf56b:
        case 0xf56c:
            return 16;
        
        default:
            return 0;
    }
}

int IbmMfmDecoder::recordMatcher(uint64_t fifo) const
{
    /* 
     * The IAM record, which is the first one on the disk (and is optional), uses
     * a distorted 0xC2 0xC2 0xC2 marker to identify it. Unfortunately, if this is
     * shifted out of phase, it becomes a legal encoding, so if we're looking at
     * real data we can't honour this. It can easily be read by keeping state as
     * to whether we're reading or seeking, but it's completely useless and so I
     * can't be bothered.
     * 
     * 0xC2 is:
     * data:    1  1  0  0  0  0  1 0   = 0xc2
     * mfm:     01 01 00 10 10 10 01 00 = 0x5254
     * special: 01 01 00 10 00 10 01 00 = 0x5224
     *                    ^^^^
     * shifted: 10 10 01 00 01 00 10 0. = legal, and might happen in real data
     * 
     * Therefore, when we've read the marker, the input fifo will contain
     * 0xXXXX522252225222.
     * 
     * All other records use 0xA1 as a marker:
     * 
     * 0xA1  is:
     * data:    1  0  1  0  0  0  0  1  = 0xa1
     * mfm:     01 00 01 00 10 10 10 01 = 0x44A9
     * special: 01 00 01 00 10 00 10 01 = 0x4489
     *                       ^^^^^
     * shifted: 10 00 10 01 00 01 00 1
     * 
     * When this is shifted out of phase, we get an illegal encoding (you
     * can't do 10 00). So, if we ever see 0x448944894489 in the input
     * fifo, we know we've landed at the beginning of a new record.
     */
         
    uint64_t masked = fifo & 0xffffffffffffLL;
    if (masked == 0x448944894489LL)
        return 48;
    return 0;
}
#endif
