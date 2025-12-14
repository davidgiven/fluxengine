#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/kryoflux.h"

static void test_convert(const Bytes& kryofluxbytes, const Bytes& fluxmapbytes)
{
    std::unique_ptr<Fluxmap> fluxmap = readStream(kryofluxbytes);

    if (fluxmap->rawBytes() != fluxmapbytes)
    {
        std::cout << "Kryoflux bytes:" << std::endl;
        hexdump(std::cout, kryofluxbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, fluxmap->rawBytes());
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, fluxmapbytes);
        abort();
    }
}

static void test_stream_reader()
{
    test_convert(Bytes{}, Bytes{});

    /* Simple one-byte intervals */
    test_convert(Bytes{0x20, 0x20, 0x20, 0x20}, Bytes{0x8f, 0x8f, 0x8f, 0x8f});

    /* One-and-a-half-byte intervals */
    test_convert(Bytes{0x20, 0x00, 0x10, 0x20, 0x01, 0x10, 0x20},
        Bytes{0x8f, 0x87, 0x8f, 0x3f, 0x3f, 0x89, 0x8f});

    /* Two-byte intervals */
    test_convert(Bytes{0x20, 0x0c, 0x00, 0x10, 0x20, 0x0c, 0x01, 0x10, 0x20},
        Bytes{0x8f, 0x87, 0x8f, 0x3f, 0x3f, 0x89, 0x8f});

    /* Overflow */
    test_convert(Bytes{0x20, 0x0b, 0x10, 0x20},
        Bytes{0x8f} + (Bytes{0x3f} * 0x207) + Bytes{0xa9, 0x8f});

    /* Single-byte nop */
    test_convert(Bytes{0x20, 0x08, 0x20}, Bytes{0x8f, 0x8f});

    /* Double-byte nop */
    test_convert(Bytes{0x20, 0x09, 0xde, 0x20}, Bytes{0x8f, 0x8f});

    /* Triple-byte nop */
    test_convert(Bytes{0x20, 0x0a, 0xde, 0xad, 0x20}, Bytes{0x8f, 0x8f});

    /* OOB block */
    test_convert(
        Bytes{
            0x20, /* data before */
            0x0d, /* OOB */
            0xaa, /* type byte */
            0x01,
            0x00, /* size of payload, little-endian */
            0x55, /* payload */
            0x20  /* data continues */
        },
        Bytes{0x8f, 0x8f});
}

int main(int argc, const char* argv[])
{
    test_stream_reader();
    return 0;
}
