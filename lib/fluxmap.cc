#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendBytes(const Bytes& bytes)
{
	if (bytes.size() == 0)
		return *this;

    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    ByteWriter bw(_bytes);
    bw.seekToEnd();

    while (len--)
    {
        uint8_t byte = *ptr++;
		_ticks += byte & 0x3f;
        bw.write_8(byte);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

uint8_t& Fluxmap::findLastByte()
{
	if (_bytes.empty())
		appendByte(0x00);
	return *(_bytes.end() - 1);
}

Fluxmap& Fluxmap::appendInterval(uint32_t ticks)
{
    while (ticks >= 0x3f)
    {
        appendByte(0x3f);
        ticks -= 0x3f;
    }
    appendByte((uint8_t)ticks);
    return *this;
}

Fluxmap& Fluxmap::appendPulse()
{
	findLastByte() |= 0x80;
    return *this;
}

Fluxmap& Fluxmap::appendIndex()
{
	findLastByte() |= 0x40;
    return *this;
}

void Fluxmap::precompensate(int threshold_ticks, int amount_ticks)
{

/*     PRECOMP 432PRECOMPENSATION DELAY https://www.mouser.com/datasheet/2/268/37c78-468028.pdf
    111     0.00 ns-DISABLED
    001     41.67 ns
    010     83.34 ns 
    011     125.00 ns 
    100     166.67 ns 
    101     208.33 ns 
    110     250.00 ns 
    000     Default (See Table 12)

    Table 12 - Default Precompensation Delays
    DATA RATEPRECOMPENSATION DELAYS
    2 Mbps      20.8 ns 
    1 Mbps      41.67 ns 
    500 Kbps    125 ns 
    300 Kbps    125 ns 
    250 Kbps    125 ns
 */    
    uint8_t junk = 0xff;

    for (unsigned i=0; i<_bytes.size(); i++)
    {
        uint8_t& prev = (i == 0) ? junk : _bytes[i-1];
		uint8_t prevticks = prev & 0x3f;
        uint8_t currticks = _bytes[i] & 0x3f;

        if (currticks < (3*threshold_ticks))
        {
            if ((prevticks <= threshold_ticks) && (currticks > threshold_ticks))
            {
                /* 01001; move the previous bit backwards. */
                if (prevticks >= (1+amount_ticks))
                    prev -= amount_ticks;
                if (currticks <= (0x7f-amount_ticks))
                    currticks += amount_ticks;
            }
            else if ((prevticks > threshold_ticks) && (currticks <= threshold_ticks))
            {
                /* 00101; move the current bit forwards. */
                if (prevticks <= (0x7f-amount_ticks))
                    prev += amount_ticks;
                if (currticks >= (1+amount_ticks))
                    currticks -= amount_ticks;
            }
        }
    }
}
