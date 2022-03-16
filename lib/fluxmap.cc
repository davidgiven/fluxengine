#include "globals.h"
#include "fluxmap.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendBytes(const Bytes& bytes)
{
    if (bytes.size() == 0)
        return *this;

    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    ByteWriter bw(_bytes);
    bw.seekToEnd();

    while (len--)
    {
        uint8_t byte = *ptr++;
        _ticks += byte & 0x3f;
        bw.write_8(byte);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

uint8_t& Fluxmap::findLastByte()
{
    if (_bytes.empty())
        appendByte(0x00);
    return *(_bytes.end() - 1);
}

Fluxmap& Fluxmap::appendInterval(uint32_t ticks)
{
    while (ticks >= 0x3f)
    {
        appendByte(0x3f);
        ticks -= 0x3f;
    }
    appendByte((uint8_t)ticks);
    return *this;
}

Fluxmap& Fluxmap::appendPulse()
{
    findLastByte() |= 0x80;
    return *this;
}

Fluxmap& Fluxmap::appendIndex()
{
    findLastByte() |= 0x40;
    return *this;
}

Fluxmap& Fluxmap::appendDesync()
{
    appendByte(F_DESYNC);
    return *this;
}

void Fluxmap::precompensate(int threshold_ticks, int amount_ticks)
{
    uint8_t junk = 0xff;

    for (unsigned i = 0; i < _bytes.size(); i++)
    {
        uint8_t& prev = (i == 0) ? junk : _bytes[i - 1];
        uint8_t prevticks = prev & 0x3f;
        uint8_t currticks = _bytes[i] & 0x3f;

        if (currticks < (3 * threshold_ticks))
        {
            if ((prevticks <= threshold_ticks) && (currticks > threshold_ticks))
            {
                /* 01001; move the previous bit backwards. */
                if (prevticks >= (1 + amount_ticks))
                    prev -= amount_ticks;
                if (currticks <= (0x7f - amount_ticks))
                    currticks += amount_ticks;
            }
            else if ((prevticks > threshold_ticks) &&
                     (currticks <= threshold_ticks))
            {
                /* 00101; move the current bit forwards. */
                if (prevticks <= (0x7f - amount_ticks))
                    prev += amount_ticks;
                if (currticks >= (1 + amount_ticks))
                    currticks -= amount_ticks;
            }
        }
    }
}

std::vector<std::unique_ptr<const Fluxmap>> Fluxmap::split() const
{
    std::vector<std::unique_ptr<const Fluxmap>> maps;
    auto bytesVector = rawBytes().split(F_DESYNC);

    for (auto bytes : bytesVector)
    {
        if (bytes.size() != 0)
            maps.push_back(std::move(std::make_unique<Fluxmap>(bytes)));
    }

    return maps;
}

std::unique_ptr<const Fluxmap> Fluxmap::rescale(double scale) const
{
    if (scale == 1.0)
        return std::make_unique<Fluxmap>(rawBytes());

    auto newFluxmap = std::make_unique<Fluxmap>();
    int lastEvent = 0;
	for (uint8_t b : _bytes)
    {
        lastEvent += b & 0x3f;
        if (b & 0xc0)
        {
            newFluxmap->appendInterval(lastEvent * scale + 0.5);
            newFluxmap->findLastByte() |= b & 0xc0;
            lastEvent = 0;
        }
    }

    return newFluxmap;
}
