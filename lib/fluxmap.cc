#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendBytes(const Bytes& bytes)
{
    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    ByteWriter bw(_bytes);
    bw.seekToEnd();

    while (len--)
    {
        uint8_t byte = *ptr++;
        if (byte < 0x80)
            _ticks += byte;
        bw.write_8(byte);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

Fluxmap& Fluxmap::appendInterval(uint32_t ticks)
{
    while (ticks >= 0x7f)
    {
        appendByte(0x7f);
        ticks -= 0x7f;
    }
    appendByte((uint8_t)ticks);
    return *this;
}

Fluxmap& Fluxmap::appendPulse()
{
    appendByte(0x80);
    return *this;
}

Fluxmap& Fluxmap::appendIndex()
{
    appendByte(0x81);
    return *this;
}

void Fluxmap::precompensate(int threshold_ticks, int amount_ticks)
{
    uint8_t junk = 0xff;

    for (unsigned i=0; i<_bytes.size(); i++)
    {
        uint8_t& prev = (i == 0) ? junk : _bytes[i-1];
        uint8_t curr = _bytes[i];

        if (curr < (3*threshold_ticks))
        {
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
}
