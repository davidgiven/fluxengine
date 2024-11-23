#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"

Bytes decodeFmMfm(
    std::vector<bool>::const_iterator ii, std::vector<bool>::const_iterator end)
{
    /*
     * FM is dumb as rocks, consisting on regular clock pulses with data pulses
     * in the gaps. 0x00 is:
     *
     *     X-X-X-X-X-X-X-X-
     *
     * 0xff is:
     *
     *     XXXXXXXXXXXXXXXX
     *
     * So we just need to extract all the odd bits.
     *
     * MFM and M2FM are slightly more complicated, where the first bit of each
     * pair can be either 0 or 1... but the second bit is always the data bit,
     * and at this point we simply don't care what the first bit is, so
     * decoding MFM uses just the same code!
     */

    Bytes bytes;
    ByteWriter bw(bytes);

    int bitcount = 0;
    uint8_t fifo = 0;

    while (ii != end)
    {
        ii++; /* skip clock bit */
        if (ii == end)
            break;
        fifo = (fifo << 1) | *ii++;

        bitcount++;
        if (bitcount == 8)
        {
            bw.write_8(fifo);
            bitcount = 0;
        }
    }

    if (bitcount != 0)
    {
        fifo <<= 8 - bitcount;
        bw.write_8(fifo);
    }

    return bytes;
}

void encodeFm(std::vector<bool>& bits, unsigned& cursor, const Bytes& input)
{
    if (bits.size() == 0)
        return;
    unsigned len = bits.size() - 1;

    for (uint8_t b : input)
    {
        for (int i = 0; i < 8; i++)
        {
            bool bit = b & 0x80;
            b <<= 1;

            if (cursor >= len)
                return;

            bits[cursor++] = true;
            bits[cursor++] = bit;
        }
    }
}

void encodeMfm(std::vector<bool>& bits,
    unsigned& cursor,
    const Bytes& input,
    bool& lastBit)
{
    if (bits.size() == 0)
        return;
    unsigned len = bits.size() - 1;

    for (uint8_t b : input)
    {
        for (int i = 0; i < 8; i++)
        {
            bool bit = b & 0x80;
            b <<= 1;

            if (cursor >= len)
                return;

            bits[cursor++] = !lastBit && !bit;
            bits[cursor++] = bit;
            lastBit = bit;
        }
    }
}

Bytes encodeMfm(const Bytes& input, bool& lastBit)
{
    ByteReader br(input);
    BitReader bitr(br);
    Bytes b;
    ByteWriter bw(b);
    BitWriter bitw(bw);

    while (!bitr.eof())
    {
        uint8_t bit = bitr.get();

        bitw.push(!lastBit && !bit);
        bitw.push(bit);
        lastBit = bit;
    }

    bitw.flush();
    return b;
}
