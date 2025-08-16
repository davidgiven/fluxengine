#pragma once

class TrackHead
{
public:
    bool operator==(const TrackHead&) const = default;

    unsigned track;
    unsigned head;
};

extern std::vector<TrackHead> parseTrackHeadsString(const std::string& s);
