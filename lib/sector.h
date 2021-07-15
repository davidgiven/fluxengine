#ifndef SECTOR_H
#define SECTOR_H

#include "bytes.h"
#include "fluxmap.h"

/* 
 * Note that sectors here used zero-based numbering throughout (to make the
 * maths easier); traditionally floppy disk use 0-based track numbering and
 * 1-based sector numbering, which makes no sense.
 */
class Sector
{
public:
	enum Status
	{
		OK,
		BAD_CHECKSUM,
        MISSING,
        DATA_MISSING,
        CONFLICT,
        INTERNAL_ERROR
	};

    static const std::string statusToString(Status status);
    static Status stringToStatus(const std::string& value);

	Status status = Status::INTERNAL_ERROR;
    Fluxmap::Position position;
    nanoseconds_t clock = 0;
    nanoseconds_t headerStartTime = 0;
    nanoseconds_t headerEndTime = 0;
    nanoseconds_t dataStartTime = 0;
    nanoseconds_t dataEndTime = 0;
    int physicalCylinder = 0;
    int physicalHead = 0;
    int logicalTrack = 0;
    int logicalSide = 0;
    int logicalSector = 0;
    Bytes data;
};

inline Sector& getSector(const std::set<std::shared_ptr<Sector>>& sectors,
		unsigned track, unsigned side, unsigned sector);

#endif

