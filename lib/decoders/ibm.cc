#include "globals.h"
#include "decoders.h"
#include "image.h"
#include <string.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value);

std::vector<std::unique_ptr<Sector>> decodeIbmRecordsToSectors(const std::vector<std::vector<uint8_t>>& records)
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
                    goto garbage;
                memcpy(&idam, &record[0], sizeof(idam));
                idamValid = true;
                /* TODO: check CRC! */
                break;
            }

            case IBM_DAM1:
            case IBM_DAM2:
            {
                if (!idamValid)
                    goto garbage;
                
                unsigned size = 1 << (idam.sectorSize + 7);
                if ((record.size()-IBM_DAM_LEN) < size)
                    goto garbage;
                /* TODO: check CRC! */

                std::vector<uint8_t> sectordata(size);
                memcpy(&sectordata[0], &record[4], size);

                auto sector = std::unique_ptr<Sector>(new Sector(idam.cylinder, idam.side, idam.sector-1, sectordata));
                sectors.push_back(std::move(sector));
                idamValid = false;
                break;
            }

            default:
            garbage:
                Error() << "garbage record on disk (this diagnostic needs improving)";
        }
    }

    return sectors;
}
