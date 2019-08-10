#include "globals.h"
#include "bytes.h"
#include "fmt/format.h"
#include "common/crunch.h"
#include <fstream>
#include <zlib.h>

static std::shared_ptr<std::vector<uint8_t>> createVector(unsigned size)
{
    return std::make_shared<std::vector<uint8_t>>(size);
}

static std::shared_ptr<std::vector<uint8_t>> createVector(const uint8_t* ptr, unsigned size)
{
    auto vector = std::make_shared<std::vector<uint8_t>>(size);
    std::uninitialized_copy(ptr, ptr+size, vector->begin());
    return vector;
}

static std::shared_ptr<std::vector<uint8_t>> createVector(std::initializer_list<uint8_t> data)
{
    auto vector = std::make_shared<std::vector<uint8_t>>(data.size());
    std::uninitialized_copy(data.begin(), data.end(), vector->begin());
    return vector;
}

Bytes::Bytes():
    _data(createVector(0)),
    _low(0),
    _high(0)
{}

Bytes::Bytes(unsigned size):
    _data(createVector(size)),
    _low(0),
    _high(size)
{}

Bytes::Bytes(const uint8_t* ptr, size_t len):
    _data(createVector(ptr, len)),
    _low(0),
    _high(len)
{}

Bytes::Bytes(std::initializer_list<uint8_t> data):
    _data(createVector(data)),
    _low(0),
    _high(data.size())
{}

Bytes::Bytes(std::shared_ptr<std::vector<uint8_t>> data):
    _data(data),
    _low(0),
    _high(data->size())
{}

Bytes::Bytes(std::shared_ptr<std::vector<uint8_t>> data, unsigned start, unsigned end):
    _data(data),
    _low(start),
    _high(end)
{}

Bytes* Bytes::operator = (const Bytes& other)
{
    _data = other._data;
    _low = other._low;
    _high = other._high;
    return this;
}

void Bytes::boundsCheck(unsigned pos) const
{
    if (pos >= _high)
        throw std::out_of_range("byte access out of range");
}

void Bytes::checkWritable()
{
    if (_data.use_count() != 1)
    {
        auto newData = createVector(size());
        std::uninitialized_copy(cbegin(), cend(), newData->begin());
        _data = newData;
        _low = 0;
        _high = newData->size();
    }
}

void Bytes::adjustBounds(unsigned pos)
{
    checkWritable();
    if (pos >= _high)
    {
        _high = pos+1;
        _data->resize(_high);
    }
}

Bytes& Bytes::resize(unsigned size)
{
    unsigned oldsize = _high - _low;
    if (size > oldsize)
    {
        checkWritable();
        _data->resize(_low + size);
    }
    _high = _low + size;
    return *this;
}

const uint8_t& Bytes::operator [] (unsigned pos) const
{
    pos += _low;
    boundsCheck(pos);
    return (*_data)[pos];
}

uint8_t& Bytes::operator [] (unsigned pos)
{
    checkWritable();
    pos += _low;
    boundsCheck(pos);
    return (*_data)[pos];
}

Bytes Bytes::slice(unsigned start, unsigned len) const
{
    start += _low;
    unsigned end = start + len;
    if (start >= _high)
    {
        /* Asking for a completely out-of-range slice --- just return zeroes. */
        return Bytes(len);
    }
    else if (end > _high)
    {
        /* Can't share the buffer, as we need to zero-pad the end. */
        Bytes b(len);
        std::uninitialized_copy(cbegin()+start, cend(), b.begin());
        return b;
    }
    else
    {
        /* Use the magic of shared_ptr to share the data. */
        Bytes b(_data, start, end);
        return b;
    }
}

uint8_t toByte(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end)
{
    return toBytes(start, end)[0];
}

Bytes toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end)
{
    Bytes bytes;
    ByteWriter bw(bytes);

    BitWriter bitw(bw);
    while (start != end)
        bitw.push(*start++, 1);
    bitw.flush();

    return bytes;
}

Bytes Bytes::swab() const
{
    Bytes output;
    ByteWriter bw(output);
    ByteReader br(*this);

    while (!br.eof())
    {
        uint8_t a = br.read_8();
        uint8_t b = br.read_8();
        bw.write_8(b);
        bw.write_8(a);
    }

    return output;
}

Bytes Bytes::compress() const
{
    uLongf destsize = compressBound(size());
    Bytes dest(destsize);
    if (::compress(dest.begin(), &destsize, cbegin(), size()) != Z_OK)
        Error() << "error compressing data";
    dest.resize(destsize);
    return dest;
}

Bytes Bytes::decompress() const
{
    Bytes output;
    ByteWriter bw(output);
    Bytes outputBuffer(1024*1024);

    z_stream stream = {};
    inflateInit(&stream);
    stream.avail_in = size();
    stream.next_in = (uint8_t*) cbegin();

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
        
        bw += outputBuffer.slice(0, outputBuffer.size() - stream.avail_out);
    }
    while (ret != Z_STREAM_END);
    inflateEnd(&stream);

    return output;
}

Bytes Bytes::crunch() const
{
    Bytes output;
    ByteWriter bw(output);
    Bytes outputBuffer(1024*1024);

    crunch_state_t cs = {};
    cs.inputptr = begin();
    cs.inputlen = size();

    do
    {
        cs.outputptr = outputBuffer.begin();
        cs.outputlen = outputBuffer.size();

        ::crunch(&cs);
        bw += outputBuffer.slice(0, outputBuffer.size() - cs.outputlen);
    }
    while (cs.inputlen != 0);
    cs.outputptr = outputBuffer.begin();
    cs.outputlen = outputBuffer.size();
    donecrunch(&cs);
    bw += outputBuffer.slice(0, outputBuffer.size() - cs.outputlen);

    return output;
}

Bytes Bytes::uncrunch() const
{
    Bytes output;
    ByteWriter bw(output);
    Bytes outputBuffer(1024*1024);

    crunch_state_t cs = {};
    cs.inputptr = begin();
    cs.inputlen = size();

    do
    {
        cs.outputptr = outputBuffer.begin();
        cs.outputlen = outputBuffer.size();

        ::uncrunch(&cs);
        bw += outputBuffer.slice(0, outputBuffer.size() - cs.outputlen);
    }
    while (cs.inputlen != 0);
    cs.outputptr = outputBuffer.begin();
    cs.outputlen = outputBuffer.size();
    doneuncrunch(&cs);
    bw += outputBuffer.slice(0, outputBuffer.size() - cs.outputlen);

    return output;
}

void Bytes::writeToFile(const std::string& filename) const
{
    std::ofstream f(filename, std::ios::out | std::ios::binary);
    if (!f.is_open())
        Error() << fmt::format("cannot open output file '{}'", filename);

    f.write((const char*) cbegin(), size());
    f.close();
}

ByteReader Bytes::reader() const
{
    return ByteReader(*this);
}

ByteWriter Bytes::writer()
{
    return ByteWriter(*this);
}

ByteWriter& ByteWriter::operator +=(std::istream& stream)
{
    Bytes buffer(4096);

    while (stream.read((char*) buffer.begin(), buffer.size()))
        this->append(buffer);
    this->append(buffer.slice(0, stream.gcount()));
    return *this;
}

void BitWriter::push(uint32_t bits, size_t size)
{
    bits <<= 32-size;

    while (size-- != 0)
    {
        _fifo = (_fifo<<1) | (bits >> 31);
        _bitcount++;
        bits <<= 1;
        if (_bitcount == 8)
        {
            _bw.write_8(_fifo);
            _bitcount = 0;
			_fifo = 0;
        }
    }
}

void BitWriter::flush()
{
    if (_bitcount != 0)
    {
        _bw.write_8(_fifo);
        _bitcount = 0;
    }
}

std::vector<bool> reverseBits(const std::vector<bool>& bits)
{
    std::vector<bool> output(bits.rbegin(), bits.rend());
    return output;
}
