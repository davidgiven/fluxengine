#include "globals.h"
#include "decoders.h"
#include "image.h"
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
				/* TODO: check CRC! */
                break;

            case BROTHER_DATA_RECORD & 0xff:
            {
                if (record.size() < (BROTHER_DATA_RECORD_PAYLOAD+1))
                    goto garbage;
                /* TODO: check CRC! */

                std::vector<uint8_t> sectordata(BROTHER_DATA_RECORD_PAYLOAD);
                memcpy(&sectordata[0], &record[1], BROTHER_DATA_RECORD_PAYLOAD);

                auto sector = std::unique_ptr<Sector>(new Sector(Sector::OK, nextTrack, 0, nextSector, sectordata));
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
