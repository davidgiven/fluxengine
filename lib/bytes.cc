#include "globals.h"
#include "bytes.h"
#include "fmt/format.h"
#include <zlib.h>

uint8_t toByte(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end)
{
    return toBytes(start, end).at(0);
}

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
	_fifo = 0;
}

void BitAccumulator::push(uint32_t bits, size_t size)
{
    bits <<= 32-size;

    while (size-- != 0)
    {
        _fifo = (_fifo<<1) | (bits >> 31);
        _bitcount++;
        bits <<= 1;
        if (_bitcount == 8)
        {
            _data.push_back(_fifo);
            _bitcount = 0;
			_fifo = 0;
        }
    }
}

void BitAccumulator::finish()
{
    if (_bitcount != 0)
    {
        _data.push_back(_fifo);
        _bitcount = 0;
    }
}

std::vector<uint8_t> compress(const std::vector<uint8_t>& source)
{
    size_t destsize = compressBound(source.size());
    std::vector<uint8_t> dest(destsize);
    if (compress(&dest[0], &destsize, &source[0], source.size()) != Z_OK)
        Error() << "error compressing data";
    dest.resize(destsize);
    return dest;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& source)
{
    std::vector<uint8_t> output;
    std::vector<uint8_t> outputBuffer(1024*1024);

    z_stream stream = {};
    inflateInit(&stream);
    stream.avail_in = source.size();
    stream.next_in = (uint8_t*) &source[0];

    int ret;
    do
    {
        stream.avail_out = outputBuffer.size();
        stream.next_out = &outputBuffer[0];
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_BUF_ERROR)
            ret = Z_OK;
        if ((ret != Z_OK) && (ret != Z_STREAM_END))
            Error() << fmt::format(
                "failed to decompress data: {}", stream.msg ? stream.msg : "(unknown error)");
        
        output.insert(output.end(), outputBuffer.begin(), outputBuffer.end() - stream.avail_out);
    }
    while (ret != Z_STREAM_END);
    inflateEnd(&stream);

    return output;
}
