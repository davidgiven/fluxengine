#ifndef FLUXMAP_H
#define FLUXMAP_H

class Fluxmap
{
public:
    uint8_t operator[](int index) const
    {
        return _intervals.at(index);
    }

    nanoseconds_t duration() const { return _duration; }
    const uint8_t* ptr() const { return &_intervals.at(0); }
    int bytes() const { return _intervals.size(); }

    Fluxmap& appendIntervals(std::vector<uint8_t>& intervals);
    Fluxmap& appendIntervals(const uint8_t* ptr, size_t len);

    nanoseconds_t guessClock() const;

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    std::vector<uint8_t> _intervals;
};

#endif
