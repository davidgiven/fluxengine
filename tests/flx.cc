#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/flx.h"

static void test_convert(const Bytes& flxbytes, const Bytes& fluxmapbytes)
{
    std::unique_ptr<Fluxmap> fluxmap = readFlxBytes(flxbytes);

    if (fluxmap->rawBytes() != fluxmapbytes)
    {
        std::cout << "FLX bytes:" << std::endl;
        hexdump(std::cout, flxbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, fluxmap->rawBytes());
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, fluxmapbytes);
        abort();
    }
}

static void test_stream_reader()
{
    test_convert(Bytes{0}, Bytes{});

    /* Simple one-byte intervals */
    test_convert(Bytes{0, 0x64, FLX_STOP}, Bytes{0xb0});

    /* Index pulse */
    test_convert(Bytes{0, 0x64, FLX_INDEX, 0x64, FLX_STOP}, Bytes{0xf0, 0xb0});
}

int main(int argc, const char* argv[])
{
    test_stream_reader();
    return 0;
}
