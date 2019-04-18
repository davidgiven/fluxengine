#ifndef SECTOR_H
#define SECTOR_H

#include "bytes.h"

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
        CONFLICT,
        INTERNAL_ERROR
	};

    static const std::string statusToString(Status status);

	Status status = Status::INTERNAL_ERROR;
    nanoseconds_t position = 0;
    nanoseconds_t clock = 0;
    int physicalTrack = 0;
    int physicalSide = 0;
    int logicalTrack = 0;
    int logicalSide = 0;
    int logicalSector = 0;
    Bytes data;
};

#endif

