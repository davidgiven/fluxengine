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

extern std::vector<CylinderHead> parseCylinderHeadsString(const std::string& s);
extern std::string convertCylinderHeadsToString(
    const std::vector<CylinderHead>& chs);
