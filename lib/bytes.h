#ifndef BYTES_H
#define BYTES_H

class Bytes
{
public:
    Bytes();
    Bytes(unsigned size);
    Bytes(std::initializer_list<uint8_t> data);
    Bytes(std::shared_ptr<std::vector<uint8_t>> data);
    Bytes(std::shared_ptr<std::vector<uint8_t>> data, unsigned start, unsigned end);

public:
    Bytes* operator = (const Bytes& other);

public:
    /* General purpose methods */

    uint8_t operator [] (unsigned offset) const;

    unsigned size() const        { return _high - _low; }

    bool operator == (const Bytes& other) const
    { return std::equal(cbegin(), cend(), other.cbegin(), other.cend()); }

    bool operator != (const Bytes& other) const
    { return !(*this == other); }

    Bytes slice(unsigned start, unsigned len);

    unsigned tell() const        { return _pos - _low; }
    void seek(unsigned pos)      { _pos = pos + _low; }

private:
    void _boundsCheck(unsigned pos) const;
    void _writableCheck() const;
    void _adjustBounds(unsigned pos);

    /* Read-only methods */

public:
    const uint8_t* cbegin() const { return &(*_data)[_low]; }
    const uint8_t* cend() const   { return &(*_data)[_high]; }

    uint8_t read_8()
    {
        _boundsCheck(_pos);
        return (*_data)[_pos++];
    }

    uint16_t read_be16()
    {
        _boundsCheck(_pos + 1);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        return (b1<<8) | b2;
    }

    uint32_t read_be24()
    {
        _boundsCheck(_pos + 2);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        uint8_t b3 = (*_data)[_pos++];
        return (b1<<16) | (b2<<8) | b3;
    }

    uint32_t read_be32()
    {
        _boundsCheck(_pos + 3);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        uint8_t b3 = (*_data)[_pos++];
        uint8_t b4 = (*_data)[_pos++];
        return (b1<<24) | (b2<<16) | (b3<<8) | b4;
    }

    uint16_t read_le16()
    {
        _boundsCheck(_pos + 1);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        return (b2<<8) | b1;
    }

    uint32_t read_le24()
    {
        _boundsCheck(_pos + 2);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        uint8_t b3 = (*_data)[_pos++];
        return (b3<<16) | (b2<<8) | b1;
    }

    uint32_t read_le32()
    {
        _boundsCheck(_pos + 3);
        uint8_t b1 = (*_data)[_pos++];
        uint8_t b2 = (*_data)[_pos++];
        uint8_t b3 = (*_data)[_pos++];
        uint8_t b4 = (*_data)[_pos++];
        return (b4<<24) | (b3<<16) | (b2<<8) | b1;
    }

    /* Writable methods */

    uint8_t* begin() { return &(*_data)[_low]; }
    uint8_t* end()   { return &(*_data)[_high]; }

    Bytes& resize(unsigned size)
    {
        _writableCheck();
        _high = _low + size;
        _data->resize(_high);
        return *this;
    }

    Bytes& write_8(uint8_t value)
    {
        _adjustBounds(_pos);
        (*_data)[_pos++] = value;
        return *this;
    }

    Bytes& write_be16(uint16_t value)
    {
        _adjustBounds(_pos+1);
        (*_data)[_pos++] = value >> 8;
        (*_data)[_pos++] = value;
        return *this;
    }

    Bytes& write_be24(uint32_t value)
    {
        _adjustBounds(_pos+2);
        (*_data)[_pos++] = value >> 16;
        (*_data)[_pos++] = value >> 8;
        (*_data)[_pos++] = value;
        return *this;
    }

    Bytes& write_be32(uint32_t value)
    {
        _adjustBounds(_pos+3);
        (*_data)[_pos++] = value >> 24;
        (*_data)[_pos++] = value >> 16;
        (*_data)[_pos++] = value >> 8;
        (*_data)[_pos++] = value;
        return *this;
    }

    Bytes& write_le16(uint16_t value)
    {
        _adjustBounds(_pos+1);
        (*_data)[_pos++] = value;
        (*_data)[_pos++] = value >> 8;
        return *this;
    }

    Bytes& write_le24(uint32_t value)
    {
        _adjustBounds(_pos+2);
        (*_data)[_pos++] = value;
        (*_data)[_pos++] = value >> 8;
        (*_data)[_pos++] = value >> 16;
        return *this;
    }

    Bytes& write_le32(uint32_t value)
    {
        _adjustBounds(_pos+3);
        (*_data)[_pos++] = value;
        (*_data)[_pos++] = value >> 8;
        (*_data)[_pos++] = value >> 16;
        (*_data)[_pos++] = value >> 24;
        return *this;
    }

    Bytes& write(std::initializer_list<uint8_t> data)
    {
        _adjustBounds(_pos + data.size() - 1);
        std::copy(data.begin(), data.end(), &(*_data)[_pos]);
        _pos += data.size();
        return *this;
    }

private:
    std::shared_ptr<std::vector<uint8_t>> _data;
    unsigned _low;
    unsigned _high;
    unsigned _pos;
};

static inline uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

template <class T>
inline uint32_t read_be16(T ptr)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    return (ptr[0]<<8) | ptr[1];
}

template <class T>
inline uint32_t read_be24(T ptr)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    return (ptr[0]<<16) | (ptr[1]<<8) | ptr[2];
}

template <class T>
inline uint32_t read_be32(T ptr)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    return (ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3];
}

template <class T>
inline uint32_t read_le16(T ptr)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    return (ptr[1]<<8) | ptr[0];
}

template <class T>
inline uint32_t read_le32(T ptr)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    return (ptr[3]<<24) | (ptr[2]<<16) | (ptr[1]<<8) | ptr[0];
}

template <class T>
inline void write_le32(T ptr, uint32_t value)
{
    static_assert(sizeof(ptr[0]) == 1, "bad iterator type");
    ptr[0] = (value)     & 0xff;
    ptr[1] = (value>>8)  & 0xff;
    ptr[2] = (value>>16) & 0xff;
    ptr[3] = (value>>24) & 0xff;
}

extern uint8_t toByte(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

extern std::vector<uint8_t> toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

inline std::vector<uint8_t> toBytes(const std::vector<bool>& bits)
{ return toBytes(bits.begin(), bits.end()); }

extern std::vector<uint8_t> compress(const std::vector<uint8_t>& source);
extern std::vector<uint8_t> decompress(const std::vector<uint8_t>& source);

class BitAccumulator
{
public:
    BitAccumulator()
    { reset(); }

    void reset();
    void push(uint32_t bits, size_t size);
    size_t size() const { return _data.size(); }
    void finish();

    operator const std::vector<uint8_t>& ()
    { finish(); return _data; }

private:
    uint8_t _fifo;
    size_t _bitcount;
    std::vector<uint8_t> _data;
};

#endif
