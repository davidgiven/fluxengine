#include "globals.h"
#include "decoders.h"
#include "aeslanier.h"
#include "crc.h"
#include "fluxmap.h"
#include "sector.h"
#include "bytes.h"
#include <string.h>

/* Aaaah, M2FM, the encoding which time forgot! Justly so. It's like MFM, but not.
 * Data bits are written in the middle of the cell. Clock bits are written at the
 * beginnign of 
SectorVector AesLanierDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& rawrecord : rawRecords)
    {
        const auto& rawdata = rawrecord->data;
        const auto& rawbytes = toBytes(rawdata);
        const auto& bytes = decodeFmMfm(rawdata);

	}

	return sectors;
}

nanoseconds_t AesLanierDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock() / 2;
}

int AesLanierDecoder::recordMatcher(uint64_t fifo) const
{
    uint64_t masked = fifo & 0xffffffffffffULL;
    // if (masked == AMIGA_SECTOR_RECORD)
	// 	return 64;
    return 0;
}

