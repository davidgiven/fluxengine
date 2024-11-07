#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "amiga.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"

uint32_t amigaChecksum(const Bytes& bytes)
{
    ByteReader br(bytes);
    uint32_t checksum = 0;

    assert((bytes.size() & 3) == 0);
    while (!br.eof())
        checksum ^= br.read_be32();

    return checksum & 0x55555555;
}

static uint8_t everyother(uint16_t x)
{
    /* aabb ccdd eeff gghh */
    x &= 0x6666; /* 0ab0 0cd0 0ef0 0gh0 */
    x >>= 1;     /* 00ab 00cd 00ef 00gh */
    x |= x << 2; /* abab cdcd efef ghgh */
    x &= 0x3c3c; /* 00ab cd00 00ef gh00 */
    x >>= 2;     /* 0000 abcd 0000 efgh */
    x |= x >> 4; /* 0000 abcd abcd efgh */
    return x;
}

Bytes amigaInterleave(const Bytes& input)
{
    Bytes output;
    ByteWriter bw(output);

    /* Write all odd bits. (Numbering starts at 0...) */

    {
        ByteReader br(input);
        while (!br.eof())
        {
            uint16_t x = br.read_be16();
            x &= 0xaaaa;       /* a0b0 c0d0 e0f0 g0h0 */
            x |= x >> 1;       /* aabb ccdd eeff gghh */
            x = everyother(x); /* 0000 0000 abcd efgh */
            bw.write_8(x);
        }
    }

    /* Write all even bits. */

    {
        ByteReader br(input);
        while (!br.eof())
        {
            uint16_t x = br.read_be16();
            x &= 0x5555;       /* 0a0b 0c0d 0e0f 0g0h */
            x |= x << 1;       /* aabb ccdd eeff gghh */
            x = everyother(x); /* 0000 0000 abcd efgh */
            bw.write_8(x);
        }
    }

    return output;
}

Bytes amigaDeinterleave(const uint8_t*& input, size_t len)
{
    assert(!(len & 1));
    const uint8_t* odds = &input[0];
    const uint8_t* evens = &input[len / 2];
    Bytes output;
    ByteWriter bw(output);

    for (size_t i = 0; i < len / 2; i++)
    {
        uint8_t o = *odds++;
        uint8_t e = *evens++;

        /* This is the 'Interleave bits with 64-bit multiply' technique from
         * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
         */
        uint16_t result =
            (((e * 0x0101010101010101ULL & 0x8040201008040201ULL) *
                     0x0102040810204081ULL >>
                 49) &
                0x5555) |
            (((o * 0x0101010101010101ULL & 0x8040201008040201ULL) *
                     0x0102040810204081ULL >>
                 48) &
                0xAAAA);

        bw.write_be16(result);
    }

    input += len;
    return output;
}

Bytes amigaDeinterleave(const Bytes& input)
{
    const uint8_t* ptr = input.cbegin();
    return amigaDeinterleave(ptr, input.size());
}
