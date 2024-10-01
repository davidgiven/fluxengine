#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/globals.h"
#include "lib/fluxmap.h"
#include "lib/usb/applesauce.h"

static void test_convert(const Bytes& asbytes, const Bytes& flbytes)
{
    Bytes astoflbytes = applesauceToFluxEngine(asbytes);
    Bytes fltoasbytes = fluxEngineToApplesauce(flbytes);

    if (astoflbytes != flbytes)
    {
        std::cout << "Applesauce to FluxEngine conversion failed.\n";
        std::cout << "Applesauce bytes:" << std::endl;
        hexdump(std::cout, asbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, astoflbytes);
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, flbytes);
        abort();
    }

    if (fltoasbytes != asbytes)
    {
        std::cout << "FluxEngine to Applesauce conversion failed.\n";
        std::cout << "FluxEngine bytes:" << std::endl;
        hexdump(std::cout, flbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, fltoasbytes);
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, asbytes);
        abort();
    }
}

static void test_conversions()
{
    /* Simple one-byte intervals. */

    test_convert(Bytes{0x20, 0x20, 0x20, 0x20}, Bytes{0xb0, 0xb0, 0xb0, 0xb0});

    /* Long, multibyte intervals. */

    test_convert(Bytes{0xff, 0x1f, 0x20, 0xff, 0xff, 0x20},
        Bytes{0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0xb3,
            0xb0,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0x3f,
            0xb9});
}

int main(int argc, const char* argv[])
{
    test_conversions();
    return 0;
}
