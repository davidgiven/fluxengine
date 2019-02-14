#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendIntervals(const std::vector<uint8_t>& intervals)
{
    return appendIntervals(&intervals[0], intervals.size());
}

Fluxmap& Fluxmap::appendIntervals(const uint8_t* ptr, size_t len)
{
    while (len--)
    {
        uint8_t interval = *ptr++;
        _ticks += interval ? interval : 0x100;
        _intervals.push_back(interval);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}


void Fluxmap::precompensate(int threshold_ticks, int amount_ticks)
{
    uint8_t junk = 0xff;

    for (unsigned i=0; i<_intervals.size(); i++)
    {
        uint8_t& prev = (i == 0) ? junk : _intervals[i-1];
        uint8_t& curr = _intervals[i];

        if ((prev <= threshold_ticks) && (curr > threshold_ticks))
        {
            /* 01001; move the previous bit backwards. */
            prev -= amount_ticks;
            curr += amount_ticks;
        }
        else if ((prev > threshold_ticks) && (curr <= threshold_ticks))
        {
            /* 00101; move the current bit forwards. */
            prev += amount_ticks;
            curr -= amount_ticks;
        }
    }
}
