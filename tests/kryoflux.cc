#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "globals.h"
#include "fluxmap.h"
#include "fluxsource/kryoflux.h"

static Bytes operator + (const Bytes& left, const Bytes& right)
{
    Bytes output;
    ByteWriter bw(output);
    bw += left;
    bw += right;
    return output;
}

static Bytes operator * (const Bytes& left, size_t count)
{
    Bytes output;
    ByteWriter bw(output);
    while (count--)
        bw += left;
    return output;
}

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
    test_convert(
        Bytes{},
        Bytes{}
    );

    /* Simple one-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x20, 0x20, 0x20 },
        Bytes{ 0x0f, 0x80, 0x0f, 0x80, 0x0f, 0x80, 0x0f, 0x80 }
    );

    /* One-and-a-half-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x00, 0x10, 0x20, 0x01, 0x10, 0x20 },
        Bytes{ 0x0f, 0x80, 0x07, 0x80, 0x0f, 0x80, 0x7f, 0x08, 0x80, 0x0f, 0x80 }
    );

    /* Two-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x0c, 0x00, 0x10, 0x20, 0x0c, 0x01, 0x10, 0x20 },
        Bytes{ 0x0f, 0x80, 0x07, 0x80, 0x0f, 0x80, 0x7f, 0x08, 0x80, 0x0f, 0x80 }
    );

    /* Overflow */
    test_convert(
        Bytes{ 0x20, 0x0b, 0x10, 0x20 },
        Bytes{ 0x0f, 0x80 } + (Bytes{ 0x7f } * 0x101) + Bytes{ 0x63, 0x80, 0x0f, 0x80 }
    );

    /* Single-byte nop */
    test_convert(
        Bytes{ 0x20, 0x08, 0x20 },
        Bytes{ 0x0f, 0x80, 0x0f, 0x80 }
    );

    /* Double-byte nop */
    test_convert(
        Bytes{ 0x20, 0x09, 0xde, 0x20 },
        Bytes{ 0x0f, 0x80, 0x0f, 0x80 }
    );

    /* Triple-byte nop */
    test_convert(
        Bytes{ 0x20, 0x0a, 0xde, 0xad, 0x20 },
        Bytes{ 0x0f, 0x80, 0x0f, 0x80 }
    );

    /* OOB block */
    test_convert(
        Bytes{
            0x20, /* data before */
            0x0d, /* OOB */
            0xaa, /* type byte */
            0x01, 0x00, /* size of payload, little-endian */
            0x55, /* payload */
            0x20 /* data continues */
        },
        Bytes{ 0x0f, 0x80, 0x0f, 0x80 }
    );
}

int main(int argc, const char* argv[])
{
    test_stream_reader();
    return 0;
}
