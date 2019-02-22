#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

uint32_t Fluxmap::getAndIncrement(size_t& index) const
{
    uint32_t ticks = 0;

    while (index < _bytes.size())
    {
        uint8_t b = _bytes[index++];
        ticks += b;
        if (!(b & 0x80))
            break;
    }

    return ticks;
}

Fluxmap& Fluxmap::appendBytes(const std::vector<uint8_t>& bytes)
{
    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    while (len--)
    {
        uint8_t byte = *ptr++;
        _ticks += byte;
        _bytes.push_back(byte);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

Fluxmap& Fluxmap::appendInterval(uint32_t ticks)
{
    while (ticks >= 0x80)
    {
        appendByte(0x80);
        ticks -= 0x80;
    }
    appendByte((uint8_t)ticks);
    return *this;
}

void Fluxmap::precompensate(int threshold_ticks, int amount_ticks)
{
    uint8_t junk = 0xff;

    for (unsigned i=0; i<_bytes.size(); i++)
    {
        uint8_t& prev = (i == 0) ? junk : _bytes[i-1];
        uint8_t& curr = _bytes[i];

        if (curr > (3*threshold_ticks))
            continue;

        if ((prev <= threshold_ticks) && (curr > threshold_ticks))
        {
            /* 01001; move the previous bit backwards. */
            if (prev >= (1+amount_ticks))
                prev -= amount_ticks;
            if (curr <= (0x7f-amount_ticks))
                curr += amount_ticks;
        }
        else if ((prev > threshold_ticks) && (curr <= threshold_ticks))
        {
            /* 00101; move the current bit forwards. */
            if (prev <= (0x7f-amount_ticks))
                prev += amount_ticks;
            if (curr >= (1+amount_ticks))
                curr -= amount_ticks;
        }
    }
}
