#include "globals.h"
#include "decoders.h"

Bytes decodeFmMfm(const std::vector<bool> bits)
{
    /* 
     * FM is dumb as rocks, consisting on regular clock pulses with data pulses in
     * the gaps. 0x00 is:
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

    size_t cursor = 1;
    int bitcount = 0;
    uint8_t fifo;

    while (cursor < bits.size())
    {
        fifo = (fifo<<1) | bits[cursor];
        cursor += 2;
        bitcount++;
        if (bitcount == 8)
        {
            bw.write_8(fifo);
            bitcount = 0;
        }
    }

    if (bitcount != 0)
    {
        fifo <<= 8-bitcount;
        bw.write_8(fifo);
    }

    return bytes;
}
