#ifndef FLUXMAP_H
#define FLUXMAP_H

#include "lib/core/bytes.h"
#include "protocol.h"
#include "fmt/format.h"

class RawBits;

class Fluxmap
{
public:
    struct Position
    {
        unsigned bytes = 0;
        unsigned ticks = 0;
        unsigned zeroes = 0;

        nanoseconds_t ns() const
        {
            return ticks * NS_PER_TICK;
        }

        operator std::string()
        {
            return fmt::format("[b:{}, t:{}, z:{}]", bytes, ticks, zeroes);
        }
    };

public:
    Fluxmap() {}

    Fluxmap(const std::string& s)
    {
        appendBytes((const uint8_t*)s.c_str(), s.size());
    }

    Fluxmap(const Bytes bytes)
    {
        appendBytes(bytes);
    }

    nanoseconds_t duration() const
    {
        return _duration;
    }
    unsigned ticks() const
    {
        return _ticks;
    }
    size_t bytes() const
    {
        return _bytes.size();
    }
    const Bytes& rawBytes() const
    {
        return _bytes;
    }

    const uint8_t* ptr() const
    {
        if (!_bytes.empty())
            return &_bytes[0];
        return NULL;
    }

    Fluxmap& appendInterval(uint32_t ticks);
    Fluxmap& appendPulse();
    Fluxmap& appendIndex();
    Fluxmap& appendDesync();

    Fluxmap& appendBytes(const Bytes& bytes);
    Fluxmap& appendBytes(const uint8_t* ptr, size_t len);

    Fluxmap& appendByte(uint8_t byte)
    {
        return appendBytes(&byte, 1);
    }

    Fluxmap& appendBits(const std::vector<bool>& bits, nanoseconds_t clock);

    std::unique_ptr<const Fluxmap> precompensate(
        int threshold_ticks, int amount_ticks);
    std::vector<std::unique_ptr<const Fluxmap>> split() const;

private:
    uint8_t& findLastByte();

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    Bytes _bytes;
};

#endif
