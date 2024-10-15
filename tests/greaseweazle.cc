#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/greaseweazle.h"

#define E28(val)                                          \
    (1 | ((val) << 1) & 0xff), (1 | ((val) >> 6) & 0xff), \
        (1 | ((val) >> 13) & 0xff), (1 | ((val) >> 20) & 0xff)

static void test_convert(const Bytes& gwbytes, const Bytes& flbytes)
{
    Bytes gwtoflbytes = greaseweazleToFluxEngine(gwbytes, 2 * NS_PER_TICK);
    Bytes fltogwbytes = fluxEngineToGreaseweazle(flbytes, 2 * NS_PER_TICK);

    if (gwtoflbytes != flbytes)
    {
        std::cout << "Greaseweazle to FluxEngine conversion failed.\n";
        std::cout << "Greaseweazle bytes:" << std::endl;
        hexdump(std::cout, gwbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, gwtoflbytes);
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, flbytes);
        abort();
    }

    if (fltogwbytes != gwbytes)
    {
        std::cout << "FluxEngine to Greaseweazle conversion failed.\n";
        std::cout << "FluxEngine bytes:" << std::endl;
        hexdump(std::cout, flbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, fltogwbytes);
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, gwbytes);
        abort();
    }
}

static void test_conversions()
{
    /* Simple one-byte intervals. */

    test_convert(Bytes{1, 1, 1, 1, 0}, Bytes{0x82, 0x82, 0x82, 0x82});

    /* Larger one-byte intervals. */

    test_convert(Bytes{32, 0}, Bytes{0x3f, 0x81});

    test_convert(Bytes{64, 0}, Bytes{0x3f, 0x3f, 0x82});

    test_convert(Bytes{128, 0}, Bytes{0x3f, 0x3f, 0x3f, 0x3f, 0x84});

    /* Two-byte intervals. */

    test_convert(Bytes{250, 1, 0},
        Bytes{0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xbb});

    /* Very long intervals. */

    test_convert(Bytes{255, FLUXOP_SPACE, E28(2048 - 249), 249, 0},
        Bytes{0x3f} * 0x41 + Bytes{0x81});
}

int main(int argc, const char* argv[])
{
    test_conversions();
    return 0;
}
