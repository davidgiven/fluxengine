#ifndef BYTES_H
#define BYTES_H

template <class T>
uint32_t read_be16(T ptr)
{
    return (ptr[0]<<8) | ptr[1];
}

template <class T>
uint32_t read_be24(T ptr)
{
    return (ptr[0]<<16) | (ptr[1]<<8) | ptr[2];
}

template <class T>
uint32_t read_be32(T ptr)
{
    return (ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3];
}

template <class T>
uint32_t read_le16(T ptr)
{
    return (ptr[1]<<8) | ptr[0];
}

template <class T>
uint32_t read_le32(T ptr)
{
    return (ptr[3]<<24) | (ptr[2]<<16) | (ptr[1]<<8) | ptr[0];
}

extern std::vector<uint8_t> toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

std::vector<uint8_t> toBytes(const std::vector<bool>& bits)
{ return toBytes(bits.begin(), bits.end()); }

class BitAccumulator
{
public:
    BitAccumulator()
    { reset(); }

    void reset();
    void push(uint32_t bits, size_t size);
    size_t size() const { return _data.size(); }

    operator const std::vector<uint8_t>& () const { return _data; }

private:
    uint8_t _fifo;
    size_t _bitcount;
    std::vector<uint8_t> _data;
};

#endif
