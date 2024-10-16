#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/kryoflux.h"
#include "protocol.h"
#include "lib/external/flx.h"

std::unique_ptr<Fluxmap> readFlxBytes(const Bytes& bytes)
{
    ByteReader br(bytes);

    /* Skip header. */

    for (;;)
    {
        if (br.eof())
            error("malformed FLX stream");
        uint8_t b = br.read_8();
        if (b == 0)
            break;
    }

    auto fluxmap = std::make_unique<Fluxmap>();
    while (!br.eof())
    {
        uint8_t b = br.read_8();
        switch (b)
        {
            case FLX_INDEX:
                fluxmap->appendIndex();
                continue;

            case FLX_STOP:
                goto stop;

            default:
            {
                if (b < 32)
                    error("unknown FLX opcode 0x{:2x}", b);
                nanoseconds_t interval = b * FLX_TICK_NS;
                fluxmap->appendInterval(interval / NS_PER_TICK);
                fluxmap->appendPulse();
                break;
            }
        }
    }
stop:

    return fluxmap;
}
