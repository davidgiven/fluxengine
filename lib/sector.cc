#include "globals.h"
#include "sector.h"
#include "fmt/format.h"

const std::string Sector::statusToString(Status status)
{
    switch (status)
    {
        case Status::OK:           return "OK";
        case Status::BAD_CHECKSUM: return "bad checksum";
        case Status::MISSING:      return "sector not found";
        case Status::DATA_MISSING: return "present but no data found";
        case Status::CONFLICT:     return "conflicting data";
        default:                   return fmt::format("unknown error {}", status);
    }
}
