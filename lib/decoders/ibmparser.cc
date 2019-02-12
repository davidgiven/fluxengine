#include "globals.h"
#include "decoders.h"
#include "sector.h"
#include "image.h"
#include "crc.h"
#include "record.h"
#include "fmt/format.h"
#include <string.h>
#include <arpa/inet.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value);

std::vector<std::unique_ptr<Sector>> IbmRecordParser::parseRecordsToSectors(
		const RecordVector& records) const
{
    bool idamValid = false;
    IbmIdam idam;
    std::vector<std::unique_ptr<Sector>> sectors;

    unsigned prologue;
    switch (_scheme)
    {
        case IBM_SCHEME_MFM: prologue = 3; break;
        case IBM_SCHEME_FM:  prologue = 0; break;
        default: assert(false);
    }

    for (auto& record : records)
    {
        const auto& datav = record->data;
        auto data = datav.begin() + prologue;
        unsigned len = datav.size() - prologue;

        switch (data[0])
        {
            case IBM_IAM:
                /* Track header. Ignore. */
                break;

            case IBM_IDAM:
            {
                if (len < sizeof(idam))
                    break;
                memcpy(&idam, &data[0], sizeof(idam));

				uint16_t crc = crc16(CCITT_POLY, &datav[0], &datav[offsetof(IbmIdam, crc) + prologue]);
				uint16_t wantedCrc = (idam.crc[0]<<8) | idam.crc[1];
				idamValid = (crc == wantedCrc);
                break;
            }

            case IBM_DAM1:
            case IBM_DAM2:
            {
                if (!idamValid)
                    break;
                
                unsigned size = 1 << (idam.sectorSize + 7);
                if (size > (len + IBM_DAM_LEN + 2))
                    break;

                const uint8_t* userStart = &data[IBM_DAM_LEN];
                const uint8_t* userEnd = userStart + size;
				uint16_t crc = crc16(CCITT_POLY, &datav[0], userEnd);
				uint16_t wantedCrc = (userEnd[0] << 8) | userEnd[1];
				int status = (crc == wantedCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                std::vector<uint8_t> sectordata(size);
                memcpy(&sectordata[0], userStart, size);

                int sectorNum = idam.sector - _sectorIdBase;
                auto sector = std::unique_ptr<Sector>(
					new Sector(status, idam.cylinder, idam.side, sectorNum, sectordata));
                sectors.push_back(std::move(sector));
                idamValid = false;
                break;
            }
        }
    }

    return sectors;
}
