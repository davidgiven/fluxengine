#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendBits(const std::vector<bool>& bits, nanoseconds_t clock)
{
	nanoseconds_t now = duration();
	for (unsigned i=0; i<bits.size(); i++)
	{
		now += clock;
		if (bits[i])
		{
			unsigned delta = (now - duration()) / NS_PER_TICK;
            appendInterval(delta);
			appendPulse();
		}
	}

	return *this;
}


