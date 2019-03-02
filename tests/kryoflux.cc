#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "globals.h"
#include "fluxmap.h"
#include "stream.h"

typedef std::vector<uint8_t> bvector;

struct memstreambuf : std::streambuf
{
    memstreambuf(const bvector& vec)
    {
        this->setg((char*) &vec[0], (char*) &vec[0], (char*) &vec[0]+vec.size());
    }
};

class memstream : public std::istream
{
public:
    memstream(const bvector& vec):
        std::istream(&_buffer),
        _buffer(vec)
    {
        rdbuf(&_buffer);
    }

private:
    memstreambuf _buffer;
};

static bvector operator + (const bvector& left, const bvector& right)
{
    bvector out = left;
    out.insert(out.end(), right.begin(), right.end());
    return out;
}

static bvector operator * (const bvector& left, size_t count)
{
    bvector out;
    while (count--)
        out.insert(out.end(), left.begin(), left.end());
    return out;
}

static void test_convert(const bvector& kryofluxbytes, const bvector& fluxmapbytes)
{
    memstream stream(kryofluxbytes);
    std::unique_ptr<Fluxmap> fluxmap = readStream(stream);

    if (fluxmap->rawBytes() != fluxmapbytes)
    {
        std::cout << "Kryoflux bytes:" << std::endl;
        hexdump(std::cout, kryofluxbytes);
        std::cout << std::endl << "Produced this:" << std::endl;
        hexdump(std::cout, fluxmap->rawBytes());
        std::cout << std::endl << "Expected this:" << std::endl;
        hexdump(std::cout, fluxmapbytes);
        exit(1);
    }
}

static void test_stream_reader()
{
    test_convert(
        bvector{},
        bvector{}
    );

    /* Simple one-byte intervals */
    test_convert(
        bvector{ 0x20, 0x20, 0x20, 0x20 },
        bvector{ 0x0f, 0x0f, 0x0f, 0x0f }
    );

    /* One-and-a-half-byte intervals */
    test_convert(
        bvector{ 0x20, 0x00, 0x10, 0x20, 0x01, 0x10, 0x20 },
        bvector{ 0x0f, 0x07, 0x0f, 0x80, 0x07, 0x0f }
    );

    /* Two-byte intervals */
    test_convert(
        bvector{ 0x20, 0x0c, 0x00, 0x10, 0x20, 0x0c, 0x01, 0x10, 0x20 },
        bvector{ 0x0f, 0x07, 0x0f, 0x80, 0x07, 0x0f }
    );

    /* Overflow */
    test_convert(
        bvector{ 0x20, 0x0b, 0x10, 0x20 },
        bvector{ 0x0f } + (bvector{ 0x80 } * 0xff) + bvector{ 0x62, 0x0f }
    );

    /* Single-byte nop */
    test_convert(
        bvector{ 0x20, 0x08, 0x20 },
        bvector{ 0x0f, 0x0f }
    );

    /* Double-byte nop */
    test_convert(
        bvector{ 0x20, 0x09, 0xde, 0x20 },
        bvector{ 0x0f, 0x0f }
    );

    /* Triple-byte nop */
    test_convert(
        bvector{ 0x20, 0x0a, 0xde, 0xad, 0x20 },
        bvector{ 0x0f, 0x0f }
    );

    /* OOB block */
    test_convert(
        bvector{
            0x20, /* data before */
            0x0d, /* OOB */
            0xaa, /* type byte */
            0x01, 0x00, /* size of payload, little-endian */
            0x55, /* payload */
            0x20 /* data continues */
        },
        bvector{ 0x0f, 0x0f }
    );
}

int main(int argc, const char* argv[])
{
    test_stream_reader();
    return 0;
}
