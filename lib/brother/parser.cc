#include "globals.h"
#include "decoders.h"
#include "sector.h"
#include "image.h"
#include "brother.h"
#include "crc.h"
#include "record.h"
#include <string.h>

std::vector<std::unique_ptr<Sector>> BrotherRecordParser::parseRecordsToSectors(
        const RecordVector& records)
{
	int nextTrack = 0;
	int nextSector = 0;
    bool hasHeader = false;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
		const std::vector<uint8_t>& data = record->data;
        switch (data[0])
        {
            case BROTHER_SECTOR_RECORD & 0xff:
				if (data.size() < 3)
					goto garbage;
				nextTrack = data[1];
				nextSector = data[2];
				hasHeader = true;
                break;

            case BROTHER_DATA_RECORD & 0xff:
            {
                if (data.size() < (BROTHER_DATA_RECORD_PAYLOAD+1))
                    goto garbage;
				if (!hasHeader)
					goto garbage;
				uint32_t realCrc = crcbrother(&data[1], &data[257]);
				uint32_t wantCrc = (data[257]<<16) | (data[258]<<8) | data[259];
				int status = (realCrc == wantCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                std::vector<uint8_t> sectordata(BROTHER_DATA_RECORD_PAYLOAD);
                memcpy(&sectordata[0], &data[1], BROTHER_DATA_RECORD_PAYLOAD);

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
