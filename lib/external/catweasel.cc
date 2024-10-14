#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "lib/external/catweasel.h"

std::unique_ptr<Fluxmap> decodeCatweaselData(
    const Bytes& bytes, nanoseconds_t clock)
{
    std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
    uint32_t pending = 0;
    bool oldindex = true;
    const uint8_t* ptr = bytes.begin();
    while (ptr != bytes.end())
    {
        uint32_t b = *ptr++;
        bool index = !!(b & 0x80);
        b &= 0x7f;
        if (b == 0x7f)
        {
            pending += 0x7f;
            continue;
        }
        b += pending;
        pending = 0;

        double interval_ns = b * clock;
        fluxmap->appendInterval(interval_ns / NS_PER_TICK);
        fluxmap->appendPulse();

        if (index && !oldindex)
            fluxmap->appendIndex();
        oldindex = index;
    }

    return fluxmap;
}
