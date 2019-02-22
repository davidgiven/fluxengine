#ifndef FLUXMAP_H
#define FLUXMAP_H

class Fluxmap
{
public:
    uint32_t getAndIncrement(size_t& index) const;

    nanoseconds_t duration() const { return _duration; }
    size_t bytes() const { return _bytes.size(); }

    const uint8_t* ptr() const
	{
		if (!_bytes.empty())
			return &_bytes.at(0);
		return NULL;
	}

    Fluxmap& appendBytes(const std::vector<uint8_t>& bytes);
    Fluxmap& appendBytes(const uint8_t* ptr, size_t len);

    Fluxmap& appendByte(uint8_t byte)
    {
        return appendBytes(&byte, 1);
    }

    nanoseconds_t guessClock() const;
	std::vector<bool> decodeToBits(nanoseconds_t clock_period) const;

	Fluxmap& appendBits(const std::vector<bool>& bits, nanoseconds_t clock);

	void precompensate(int threshold_ticks, int amount_ticks);

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    std::vector<uint8_t> _bytes;
};

#endif
