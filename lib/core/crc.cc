#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/core/crc.h"

template <class T>
T reflect(T bin, unsigned width = sizeof(T) * 8)
{
    T bout = 0;
    while (width--)
    {
        bout <<= 1;
        bout |= (bin & 1);
        bin >>= 1;
    }
    return bout;
}

uint64_t generic_crc(const struct crcspec& spec, const Bytes& bytes)
{
    uint64_t crc = spec.init;
    uint64_t top = 1LL << (spec.width - 1);
    uint64_t mask = (top << 1) - 1;

    for (uint8_t b : bytes)
    {
        if (spec.refin)
            b = reflect(b);

        for (uint8_t i = 0x80; i != 0; i >>= 1)
        {
            uint64_t bit = crc & top;
            crc <<= 1;
            if (b & i)
                bit ^= top;
            if (bit)
                crc ^= spec.poly;
        }
    }

    if (spec.refout)
        crc = reflect(crc, spec.width);
    crc ^= spec.xorout;
    return crc & mask;
}

uint16_t sumBytes(const Bytes& bytes)
{
    ByteReader br(bytes);

    uint16_t i = 0;
    while (!br.eof())
        i += br.read_8();
    return i;
}

uint8_t xorBytes(const Bytes& bytes)
{
    ByteReader br(bytes);

    uint8_t i = 0;
    while (!br.eof())
        i ^= br.read_8();
    return i;
}

uint16_t crc16(uint16_t poly, uint16_t crc, const Bytes& bytes)
{
    ByteReader br(bytes);

    while (!br.eof())
    {
        crc ^= br.read_8() << 8;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? ((crc << 1) ^ poly) : (crc << 1);
    }

    return crc;
}

uint16_t crc16ref(uint16_t poly, uint16_t crc, const Bytes& bytes)
{
    ByteReader br(bytes);

    while (!br.eof())
    {
        crc ^= br.read_8();
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x0001) ? ((crc >> 1) ^ poly) : (crc >> 1);
    }

    return crc;
}

/* Thanks to user202729 on StackOverflow for miraculously reverse engineering
 * this. */
uint32_t crcbrother(const Bytes& bytes)
{
    ByteReader br(bytes);

    uint32_t crc = br.read_8();
    while (!br.eof())
    {
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x800000) ? ((crc << 1) ^ BROTHER_POLY) : (crc << 1);
        crc ^= br.read_8();
    }

    return crc & 0xFFFFFF;
}
