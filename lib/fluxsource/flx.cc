#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "protocol.h"
#include "lib/fluxsource/flx.h"
#include "fmt/format.h"

std::unique_ptr<Fluxmap> readFlxBytes(const Bytes& bytes)
{
    ByteReader br(bytes);

    /* Skip header. */

    for (;;)
    {
        if (br.eof())
            Error() << fmt::format("malformed FLX stream");
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
                    Error() << fmt::format("unknown FLX opcode 0x{:2x}", b);
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
