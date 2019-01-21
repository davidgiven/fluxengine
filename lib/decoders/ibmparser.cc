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
		const RecordVector& records)
{
    bool idamValid = false;
    IbmIdam idam;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
		const std::vector<uint8_t>& data = record->data;
        switch (data[3])
        {
            case IBM_IAM:
                /* Track header. Ignore. */
                break;

            case IBM_IDAM:
            {
                if (data.size() < sizeof(idam))
                    break;
                memcpy(&idam, &data[0], sizeof(idam));

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
                if ((data.size()-IBM_DAM_LEN-2) < size)
                    break;

				uint16_t crc = crc16(CCITT_POLY, &data[0], &data[size+4]);
				uint16_t wantedCrc = (data[size+4] << 8) | data[size+5];
				int status = (crc == wantedCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                std::vector<uint8_t> sectordata(size);
                memcpy(&sectordata[0], &data[4], size);

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
