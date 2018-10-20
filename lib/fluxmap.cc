#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendIntervals(std::vector<uint8_t>& intervals)
{
    return appendIntervals(&_intervals[0], _intervals.size());
}

Fluxmap& Fluxmap::appendIntervals(const uint8_t* ptr, size_t len)
{
    _intervals.reserve(_intervals.size() + len);

    while (len--)
    {
        uint8_t interval = *ptr++;
        _ticks += interval ? interval : 0x100;
        _intervals.push_back(interval);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

