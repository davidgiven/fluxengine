#pragma once

struct CylinderHead
{
    bool operator==(const CylinderHead&) const = default;

    unsigned cylinder, head;
};

extern std::vector<CylinderHead> parseCylinderHeadsString(const std::string& s);
