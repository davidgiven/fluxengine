#ifndef FLUXMAP_H
#define FLUXMAP_H

#include "bytes.h"

class RawBits;

class Fluxmap
{
public:
    nanoseconds_t duration() const { return _duration; }
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

    nanoseconds_t guessClock() const;
	const RawBits decodeToBits(nanoseconds_t clock_period) const;

	Fluxmap& appendBits(const std::vector<bool>& bits, nanoseconds_t clock);

	void precompensate(int threshold_ticks, int amount_ticks);

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    Bytes _bytes;
};

class FluxmapReader
{
public:
    FluxmapReader(const Fluxmap& fluxmap):
        _fluxmap(fluxmap),
        _bytes(fluxmap.ptr()),
        _size(fluxmap.bytes())
    {
        rewind();
    }

    FluxmapReader(const Fluxmap&& fluxmap) = delete;

    void rewind()
    { _cursor = 0; }

    size_t tell() const
    { return _cursor; }

    int read(unsigned& ticks);
    int readPulse(unsigned& ticks);

private:
    const Fluxmap& _fluxmap;
    const uint8_t* _bytes;
    const size_t _size;
    size_t _cursor;
};

#endif
