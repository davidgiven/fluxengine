#include "globals.h"
#include "bytes.h"

std::vector<uint8_t> toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end)
{
    std::vector<uint8_t> bytes;
    size_t bitcount = 0;
    uint8_t fifo;

    while (start != end)
    {
        fifo = (fifo<<1) | *start++;
        bitcount++;
        if (bitcount == 8)
        {
            bitcount = 0;
            bytes.push_back(fifo);
        }
    }

    if (bitcount != 0)
    {
        fifo <<= 8-bitcount;
        bytes.push_back(fifo);
    }

    return bytes;
}

void BitAccumulator::reset()
{
    _data.resize(0);
    _bitcount = 0;
}

void BitAccumulator::push(uint32_t bits, size_t size)
{
    bits <<= 32-size;

    while (size-- != 0)
    {
        _fifo = (_fifo<<1) | (bits >> 31);
        _bitcount++;
        if (_bitcount == 8)
        {
            _data.push_back(_fifo);
            _bitcount = 0;
        }
    }
}

