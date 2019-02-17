#include "globals.h"
#include "decoders.h"
#include "sector.h"
#include "image.h"
#include "amiga.h"
#include "crc.h"
#include "record.h"
#include <string.h>

std::vector<std::unique_ptr<Sector>> AmigaRecordParser::parseRecordsToSectors(
        const RecordVector& records) const
{
	int nextTrack = 0;
	int nextSector = 0;
    bool hasHeader = false;
    std::vector<std::unique_ptr<Sector>> sectors;

    for (auto& record : records)
    {
		const std::vector<uint8_t>& data = record->data;
    }

    return sectors;
}
