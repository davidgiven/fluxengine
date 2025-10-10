#pragma once

struct CylinderHead
{
    bool operator==(const CylinderHead&) const = default;
    std::strong_ordering operator<=>(const CylinderHead&) const = default;

    unsigned cylinder, head;
};

struct CylinderHeadSector
{
    bool operator==(const CylinderHeadSector&) const = default;
    std::strong_ordering operator<=>(const CylinderHeadSector&) const = default;

    unsigned cylinder, head, sector;
};

struct LogicalLocation
{
    bool operator==(const LogicalLocation&) const = default;
    std::strong_ordering operator<=>(const LogicalLocation&) const = default;

    unsigned logicalCylinder;
    unsigned logicalHead;
    unsigned logicalSector;

    operator std::string() const
    {
        return fmt::format(
            "c{}h{}s{}", logicalCylinder, logicalHead, logicalSector);
    }

    CylinderHead trackLocation() const
    {
        return {logicalCylinder, logicalHead};
    }
};

inline std::ostream& operator<<(std::ostream& stream, LogicalLocation location)
{
    stream << (std::string)location;
    return stream;
}

extern std::vector<CylinderHead> parseCylinderHeadsString(const std::string& s);
extern std::string convertCylinderHeadsToString(
    const std::vector<CylinderHead>& chs);
