#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "globals.h"
#include "fluxmap.h"
#include "stream.h"

struct memstreambuf : std::streambuf
{
    memstreambuf(const Bytes& bytes)
    {
        this->setg((char*) bytes.begin(), (char*) bytes.begin(), (char*) bytes.end());
    }
};

class memstream : public std::istream
{
public:
    memstream(const Bytes& bytes):
        std::istream(&_buffer),
        _buffer(bytes)
    {
        rdbuf(&_buffer);
    }

private:
    memstreambuf _buffer;
};

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
        Bytes{},
        Bytes{}
    );

    /* Simple one-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x20, 0x20, 0x20 },
        Bytes{ 0x0f, 0x0f, 0x0f, 0x0f }
    );

    /* One-and-a-half-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x00, 0x10, 0x20, 0x01, 0x10, 0x20 },
        Bytes{ 0x0f, 0x07, 0x0f, 0x80, 0x07, 0x0f }
    );

    /* Two-byte intervals */
    test_convert(
        Bytes{ 0x20, 0x0c, 0x00, 0x10, 0x20, 0x0c, 0x01, 0x10, 0x20 },
        Bytes{ 0x0f, 0x07, 0x0f, 0x80, 0x07, 0x0f }
    );

    /* Overflow */
    test_convert(
        Bytes{ 0x20, 0x0b, 0x10, 0x20 },
        Bytes{ 0x0f } + (Bytes{ 0x80 } * 0xff) + Bytes{ 0x62, 0x0f }
    );

    /* Single-byte nop */
    test_convert(
        Bytes{ 0x20, 0x08, 0x20 },
        Bytes{ 0x0f, 0x0f }
    );

    /* Double-byte nop */
    test_convert(
        Bytes{ 0x20, 0x09, 0xde, 0x20 },
        Bytes{ 0x0f, 0x0f }
    );

    /* Triple-byte nop */
    test_convert(
        Bytes{ 0x20, 0x0a, 0xde, 0xad, 0x20 },
        Bytes{ 0x0f, 0x0f }
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
        Bytes{ 0x0f, 0x0f }
    );
}

int main(int argc, const char* argv[])
{
    test_stream_reader();
    return 0;
}
