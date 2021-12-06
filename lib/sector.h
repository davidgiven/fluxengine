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
    nanoseconds_t bitcell = 0;
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

	std::tuple<int, int, int, Status> key() const
	{ return std::make_tuple(logicalTrack, logicalSide, logicalSector, status); }

	bool operator == (const Sector& rhs) const
	{ return key() == rhs.key(); }

	bool operator != (const Sector& rhs) const
	{ return key() != rhs.key(); }

	bool operator < (const Sector& rhs) const
	{ return key() < rhs.key(); }

};

extern bool sectorPointerSortPredicate(std::shared_ptr<Sector>& lhs, std::shared_ptr<Sector>& rhs);
extern bool sectorPointerEqualsPredicate(std::shared_ptr<Sector>& lhs, std::shared_ptr<Sector>& rhs);

#endif

