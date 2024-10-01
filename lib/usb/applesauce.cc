#include "lib/globals.h"
#include "usb.h"
#include "protocol.h"
#include "lib/bytes.h"
#include "greaseweazle.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "lib/a2r.h"

static double a2r_to_ticks(double a2rticks)
{
    return a2rticks * A2R_NS_PER_TICK / NS_PER_TICK;
}

static double ticks_to_a2r(double flticks)
{
    return flticks * NS_PER_TICK / A2R_NS_PER_TICK;
}

Bytes fluxEngineToApplesauce(const Bytes& fldata)
{
    Fluxmap fluxmap(fldata);
    FluxmapReader fmr(fluxmap);
    Bytes asdata;
    ByteWriter bw(asdata);

    while (!fmr.eof())
    {
        unsigned ticks;
        if (!fmr.findEvent(F_BIT_PULSE, ticks))
            break;

        uint32_t applesauceTicks = ticks_to_a2r(ticks);
        while (applesauceTicks >= 255)
        {
            bw.write_8(255);
            applesauceTicks -= 255;
        }
        bw.write_8(applesauceTicks);
    }

    return asdata;
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
