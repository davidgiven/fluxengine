#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"
#include "record.h"
#include "decoders.h"
#include "sector.h"
#include "zilogmcz.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

SectorVector ZilogMczDecoder::decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack)
{
    std::vector<std::unique_ptr<Sector>> sectors;
    int nextSector;
    int nextSide;
    bool headerIsValid = false;

    for (auto& rawrecord : rawRecords)
    {
        const std::vector<bool>& rawdata = rawrecord->data;

        auto ii = rawdata.cbegin()+15;
        std::vector<bool> sliceddata(ii, ii+(0xa0*8*2));
        auto reversed = reverseBits(sliceddata);
            const auto rawbytes = decodeFmMfm(reversed);

	}

	return sectors;
}
