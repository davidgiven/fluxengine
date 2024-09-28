#include "lib/globals.h"
#include "usb.h"
#include "protocol.h"
#include "lib/bytes.h"
#include "greaseweazle.h"
#include "lib/fluxmap.h"
#include "lib/a2r.h"

static uint32_t a2r_to_ticks(uint32_t a2rticks)
{
    return a2rticks * A2R_NS_PER_TICK / NS_PER_TICK;
}

Bytes applesauceToFluxEngine(const Bytes& asdata)
{
    ByteReader br(asdata);
    Fluxmap fluxmap;

    unsigned a2rTicksSinceLastPulse = 0;
    while (!br.eof())
    {
        uint8_t b = br.read_8();
        a2rTicksSinceLastPulse += b;
        if (b != 255)
        {
            fluxmap.appendInterval(a2r_to_ticks(a2rTicksSinceLastPulse));
            fluxmap.appendPulse();
            a2rTicksSinceLastPulse = 0;
        }
    }

    return fluxmap.rawBytes();
}
