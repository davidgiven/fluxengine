#include "globals.h"
#include "decoders.h"
#include "ibm.h"
#include "crc.h"
#include "fluxmap.h"
#include "sector.h"
#include "record.h"
#include <string.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value,
		"IbmIdam is not trivially copyable");

SectorVector AbstractIbmDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    bool idamValid = false;
    IbmIdam idam;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const Bytes bytes = decodeFmMfm(rawrecord->data);
        int headerSize = skipHeaderBytes();
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

nanoseconds_t IbmMfmDecoder::guessClock(Fluxmap& fluxmap, unsigned physicalTrack) const
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
     * data:    1  1  0  0  0  0  1 0
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
     * data:    1  0  1  0  0  0  0  1
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
