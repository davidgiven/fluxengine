#ifndef BYTES_H
#define BYTES_H

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

extern std::vector<uint8_t> toBytes(
    std::vector<bool>::const_iterator start,
    std::vector<bool>::const_iterator end);

inline std::vector<uint8_t> toBytes(const std::vector<bool>& bits)
{ return toBytes(bits.begin(), bits.end()); }

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
