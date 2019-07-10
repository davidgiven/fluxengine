#ifndef FLUXMAP_H
#define FLUXMAP_H

#include "bytes.h"
#include "protocol.h"

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
        { return ticks * NS_PER_TICK; }
    };

public:
    nanoseconds_t duration() const { return _duration; }
    unsigned ticks() const { return _ticks; }
    size_t bytes() const { return _bytes.size(); }
    const Bytes& rawBytes() const { return _bytes; }

    const uint8_t* ptr() const
	{
		if (!_bytes.empty())
			return &_bytes[0];
		return NULL;
	}

    Fluxmap& appendInterval(uint32_t ticks);
    Fluxmap& appendPulse();
    Fluxmap& appendIndex();

    Fluxmap& appendBytes(const Bytes& bytes);
    Fluxmap& appendBytes(const uint8_t* ptr, size_t len);

    Fluxmap& appendByte(uint8_t byte)
    {
        return appendBytes(&byte, 1);
    }

	Fluxmap& appendBits(const std::vector<bool>& bits, nanoseconds_t clock);

	void precompensate(int threshold_ticks, int amount_ticks);

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    Bytes _bytes;
};

#endif
