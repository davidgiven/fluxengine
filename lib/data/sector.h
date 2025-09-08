#ifndef SECTOR_H
#define SECTOR_H

#include "lib/core/bytes.h"
#include "lib/data/fluxmap.h"
#include "lib/data/locations.h"

class Record;
class TrackInfo;

struct Sector : public LogicalLocation
{
    enum Status
    {
        OK,
        BAD_CHECKSUM,
        MISSING,
        DATA_MISSING,
        CONFLICT,
        INTERNAL_ERROR,
    };

    static std::string statusToString(Status status);
    static std::string statusToChar(Status status);
    static Status stringToStatus(const std::string& value);

    Status status = Status::INTERNAL_ERROR;
    std::shared_ptr<const TrackInfo> trackLayout;
    uint32_t position = 0;
    nanoseconds_t clock = 0;
    nanoseconds_t headerStartTime = 0;
    nanoseconds_t headerEndTime = 0;
    nanoseconds_t dataStartTime = 0;
    nanoseconds_t dataEndTime = 0;
    unsigned physicalCylinder = 0;
    unsigned physicalHead = 0;
    Bytes data;
    std::vector<std::shared_ptr<Record>> records;

    Sector(std::shared_ptr<const TrackInfo>& layout, const LogicalLocation& location);

    std::tuple<int, int, int, Status> key() const
    {
        return std::make_tuple(
            logicalCylinder, logicalHead, logicalSector, status);
    }

    bool operator==(const Sector& rhs) const
    {
        return key() == rhs.key();
    }

    bool operator!=(const Sector& rhs) const
    {
        return key() != rhs.key();
    }

    bool operator<(const Sector& rhs) const
    {
        return key() < rhs.key();
    }
};

template <>
struct fmt::formatter<Sector::Status> : formatter<string_view>
{
    auto format(Sector::Status status, format_context& ctx) const
    {
        return format_to(ctx.out(), "{}", Sector::statusToString(status));
    }
};

extern bool sectorPointerSortPredicate(const std::shared_ptr<const Sector>& lhs,
    const std::shared_ptr<const Sector>& rhs);
extern bool sectorPointerEqualsPredicate(
    const std::shared_ptr<const Sector>& lhs,
    const std::shared_ptr<const Sector>& rhs);

#endif
