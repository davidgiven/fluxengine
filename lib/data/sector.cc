#include "lib/core/globals.h"
#include "lib/data/flux.h"
#include "lib/data/sector.h"
#include "lib/data/layout.h"

Sector::Sector(const LogicalLocation& location):
    LogicalLocation(location),
    physicalTrack(Layout::remapTrackLogicalToPhysical(location.logicalTrack)),
    physicalSide(Layout::remapSideLogicalToPhysical(location.logicalSide))
{
}

Sector::Sector(std::shared_ptr<const TrackInfo>& layout, unsigned sectorId):
    LogicalLocation({layout->logicalTrack, layout->logicalSide, sectorId}),
    physicalTrack(layout->physicalTrack),
    physicalSide(layout->physicalSide)
{
}

std::string Sector::statusToString(Status status)
{
    switch (status)
    {
        case Status::OK:
            return "OK";
        case Status::BAD_CHECKSUM:
            return "bad checksum";
        case Status::MISSING:
            return "sector not found";
        case Status::DATA_MISSING:
            return "present but no data found";
        case Status::CONFLICT:
            return "conflicting data";
        default:
            return fmt::format("unknown error {}", status);
    }
}

std::string Sector::statusToChar(Status status)
{
    switch (status)
    {
        case Status::OK:
            return "";
        case Status::MISSING:
            return "?";
        case Status::BAD_CHECKSUM:
            return "!";
        case Status::DATA_MISSING:
            return "!";
        case Status::CONFLICT:
            return "*";
        default:
            return "?";
    }
}

Sector::Status Sector::stringToStatus(const std::string& value)
{
    if (value == "OK")
        return Status::OK;
    if (value == "bad checksum")
        return Status::BAD_CHECKSUM;
    if ((value == "sector not found") || (value == "MISSING"))
        return Status::MISSING;
    if (value == "present but no data found")
        return Status::DATA_MISSING;
    if (value == "conflicting data")
        return Status::CONFLICT;
    return Status::INTERNAL_ERROR;
}

bool sectorPointerSortPredicate(const std::shared_ptr<const Sector>& lhs,
    const std::shared_ptr<const Sector>& rhs)
{
    return *lhs < *rhs;
}

bool sectorPointerEqualsPredicate(const std::shared_ptr<const Sector>& lhs,
    const std::shared_ptr<const Sector>& rhs)
{
    if (!lhs && !rhs)
        return true;
    if (!lhs || !rhs)
        return false;
    return *lhs == *rhs;
}
