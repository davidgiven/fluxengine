#ifndef BYTES_H
#define BYTES_H

class ByteReader;
class ByteWriter;

class Bytes
{
public:
    Bytes();
    Bytes(unsigned size);
    Bytes(const uint8_t* ptr, size_t len);
    Bytes(std::initializer_list<uint8_t> data);
    Bytes(std::shared_ptr<std::vector<uint8_t>> data);
    Bytes(std::shared_ptr<std::vector<uint8_t>> data, unsigned start, unsigned end);

    Bytes* operator = (const Bytes& other);

public:
    /* General purpose methods */

    unsigned size() const        { return _high - _low; }
    bool empty() const           { return _high == _low; }

    bool operator == (const Bytes& other) const
    { return std::equal(cbegin(), cend(), other.cbegin(), other.cend()); }

    bool operator != (const Bytes& other) const
    { return !(*this == other); }

    const uint8_t& operator [] (unsigned offset) const;
    const uint8_t* cbegin() const { return &(*_data)[_low]; }
    const uint8_t* cend() const   { return &(*_data)[_high]; }
    const uint8_t* begin() const  { return &(*_data)[_low]; }
    const uint8_t* end() const    { return &(*_data)[_high]; }

    uint8_t& operator [] (unsigned offset);
    uint8_t* begin()              { checkWritable(); return &(*_data)[_low]; }
    uint8_t* end()                { checkWritable(); return &(*_data)[_high]; }

    void boundsCheck(unsigned pos) const;
    void checkWritable();
    void adjustBounds(unsigned pos);
    Bytes& resize(unsigned size);

    Bytes slice(unsigned start, unsigned len) const;
    Bytes compress() const;
    Bytes decompress() const;

    ByteReader reader() const;
    ByteWriter writer();

private:
    std::shared_ptr<std::vector<uint8_t>> _data;
    unsigned _low;
    unsigned _high;
};

class ByteReader
{
public:
    ByteReader(const Bytes& bytes):
        _bytes(bytes)
    {}

    unsigned pos = 0;
    bool eof() const
    { return pos == _bytes.size(); }

    ByteReader& seek(unsigned pos)
    {
        this->pos = pos;
        return *this;
    }

    uint8_t read_8()
    {
        return _bytes[pos++];
    }

    uint16_t read_be16()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        return (b1<<8) | b2;
    }

    uint32_t read_be24()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        uint8_t b3 = _bytes[pos++];
        return (b1<<16) | (b2<<8) | b3;
    }

    uint32_t read_be32()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        uint8_t b3 = _bytes[pos++];
        uint8_t b4 = _bytes[pos++];
        return (b1<<24) | (b2<<16) | (b3<<8) | b4;
    }

    uint16_t read_le16()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        return (b2<<8) | b1;
    }

    uint32_t read_le24()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        uint8_t b3 = _bytes[pos++];
        return (b3<<16) | (b2<<8) | b1;
    }

    uint32_t read_le32()
    {
        uint8_t b1 = _bytes[pos++];
        uint8_t b2 = _bytes[pos++];
        uint8_t b3 = _bytes[pos++];
        uint8_t b4 = _bytes[pos++];
        return (b4<<24) | (b3<<16) | (b2<<8) | b1;
    }

private:
    const Bytes& _bytes;
};

class ByteWriter
{
public:
    ByteWriter(Bytes& bytes):
        _bytes(bytes)
    {}

    unsigned pos = 0;

    ByteWriter& seek(unsigned pos)
    {
        this->pos = pos;
        return *this;
    }

    ByteWriter& seekToEnd()
    {
        pos = _bytes.size();
        return *this;
    }

    ByteWriter& write_8(uint8_t value)
    {
        _bytes.adjustBounds(pos);
        uint8_t* p = _bytes.begin();
        p[pos++] = value;
        return *this;
    }

    ByteWriter& write_be16(uint16_t value)
    {
        _bytes.adjustBounds(pos+1);
        uint8_t* p = _bytes.begin();
        p[pos++] = value >> 8;
        p[pos++] = value;
        return *this;
    }

    ByteWriter& write_be24(uint32_t value)
    {
        _bytes.adjustBounds(pos+2);
        uint8_t* p = _bytes.begin();
        p[pos++] = value >> 16;
        p[pos++] = value >> 8;
        p[pos++] = value;
        return *this;
    }

    ByteWriter& write_be32(uint32_t value)
    {
        _bytes.adjustBounds(pos+3);
        uint8_t* p = _bytes.begin();
        p[pos++] = value >> 24;
        p[pos++] = value >> 16;
        p[pos++] = value >> 8;
        p[pos++] = value;
        return *this;
    }

    ByteWriter& write_le16(uint16_t value)
    {
        _bytes.adjustBounds(pos+1);
        uint8_t* p = _bytes.begin();
        p[pos++] = value;
        p[pos++] = value >> 8;
        return *this;
    }

    ByteWriter& write_le24(uint32_t value)
    {
        _bytes.adjustBounds(pos+2);
        uint8_t* p = _bytes.begin();
        p[pos++] = value;
        p[pos++] = value >> 8;
        p[pos++] = value >> 16;
        return *this;
    }

    ByteWriter& write_le32(uint32_t value)
    {
        _bytes.adjustBounds(pos+3);
        uint8_t* p = _bytes.begin();
        p[pos++] = value;
        p[pos++] = value >> 8;
        p[pos++] = value >> 16;
        p[pos++] = value >> 24;
        return *this;
    }

    ByteWriter& operator += (std::initializer_list<uint8_t> data)
    {
        _bytes.adjustBounds(pos + data.size() - 1);
        std::copy(data.begin(), data.end(), _bytes.begin() + pos);
        pos += data.size();
        return *this;
    }

    ByteWriter& operator += (const std::vector<uint8_t>& data)
    {
        _bytes.adjustBounds(pos + data.size() - 1);
        std::copy(data.begin(), data.end(), _bytes.begin() + pos);
        pos += data.size();
        return *this;
    }

    ByteWriter& operator += (const Bytes data)
    {
        _bytes.adjustBounds(pos + data.size() - 1);
        std::copy(data.begin(), data.end(), _bytes.begin() + pos);
        pos += data.size();
        return *this;
    }

private:
    Bytes& _bytes;
};

class BitWriter
{
public:
    BitWriter(ByteWriter& bw):
        _bw(bw)
    {}

    void push(uint32_t bits, size_t size);
    void flush();

private:
    uint8_t _fifo;
    size_t _bitcount = 0;
    ByteWriter& _bw;
};

static inline uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

extern uint8_t toByte(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

extern Bytes toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

inline Bytes toBytes(const std::vector<bool>& bits)
{ return toBytes(bits.begin(), bits.end()); }

#endif
