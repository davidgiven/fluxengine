#pragma once

class Location
{
public:
    bool operator==(const Location&) const = default;

    int track;
    int head;
};

extern std::vector<Location> parseLocationsString(const std::string& s);
