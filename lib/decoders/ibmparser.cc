#include "globals.h"
#include "decoders.h"
#include "image.h"
#include "crc.h"
#include "fmt/format.h"
#include <string.h>
#include <arpa/inet.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value);

std::vector<std::unique_ptr<Sector>> parseRecordsToSectorsIbm(
		const std::vector<std::vector<uint8_t>>& records)
{
    bool idamValid = false;
    IbmIdam idam;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
        switch (record[3])
        {
            case IBM_IAM:
                /* Track header. Ignore. */
                break;

            case IBM_IDAM:
            {
                if (record.size() < sizeof(idam))
                    break;
                memcpy(&idam, &record[0], sizeof(idam));

				uint16_t crc = crc16(CCITT_POLY, (uint8_t*)&idam, (uint8_t*)&idam.crc);
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
                if ((record.size()-IBM_DAM_LEN-2) < size)
                    break;

				uint16_t crc = crc16(CCITT_POLY, &record[0], &record[size+4]);
				uint16_t wantedCrc = (record[size+4] << 8) | record[size+5];
				int status = (crc == wantedCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                std::vector<uint8_t> sectordata(size);
                memcpy(&sectordata[0], &record[4], size);

                auto sector = std::unique_ptr<Sector>(
					new Sector(status, idam.cylinder, idam.side, idam.sector-1, sectordata));
                sectors.push_back(std::move(sector));
                idamValid = false;
                break;
            }
        }
    }

    return sectors;
}
