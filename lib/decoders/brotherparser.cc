#include "globals.h"
#include "decoders.h"
#include "image.h"
#include "crc.h"
#include <string.h>

std::vector<std::unique_ptr<Sector>> parseRecordsToSectorsBrother(const std::vector<std::vector<uint8_t>>& records)
{
	int nextTrack = 0;
	int nextSector = 0;
    bool hasHeader = false;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
        switch (record[0])
        {
            case BROTHER_SECTOR_RECORD & 0xff:
				if (record.size() < 3)
					goto garbage;
				nextTrack = record[1];
				nextSector = record[2];
				hasHeader = true;
                break;

            case BROTHER_DATA_RECORD & 0xff:
            {
                if (record.size() < (BROTHER_DATA_RECORD_PAYLOAD+1))
                    goto garbage;
				if (!hasHeader)
					goto garbage;
				uint32_t realCrc = crcbrother(&record[1], &record[257]);
				uint32_t wantCrc = (record[257]<<16) | (record[258]<<8) | record[259];
				int status = (realCrc == wantCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                std::vector<uint8_t> sectordata(BROTHER_DATA_RECORD_PAYLOAD);
                memcpy(&sectordata[0], &record[1], BROTHER_DATA_RECORD_PAYLOAD);

                auto sector = std::unique_ptr<Sector>(new Sector(status, nextTrack, 0, nextSector, sectordata));
                sectors.push_back(std::move(sector));
                hasHeader = false;
                break;
            }

            default:
            garbage:
				break;
        }
    }

    return sectors;
}
