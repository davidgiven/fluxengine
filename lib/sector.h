#ifndef SECTOR_H
#define SECTOR_H

#include "bytes.h"
#include "fluxmap.h"

class Record;

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

    static std::string statusToString(Status status);
    static std::string statusToChar(Status status);
    static Status stringToStatus(const std::string& value);

	Status status = Status::INTERNAL_ERROR;
    uint32_t position;
    nanoseconds_t clock = 0;
    nanoseconds_t headerStartTime = 0;
    nanoseconds_t headerEndTime = 0;
    nanoseconds_t dataStartTime = 0;
    nanoseconds_t dataEndTime = 0;
    unsigned physicalCylinder = 0;
    unsigned physicalHead = 0;
    unsigned logicalTrack = 0;
    unsigned logicalSide = 0;
    unsigned logicalSector = 0;
    Bytes data;
	std::vector<std::shared_ptr<Record>> records;

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

