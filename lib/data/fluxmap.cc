#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "protocol.h"
#include <mutex>

Fluxmap& Fluxmap::appendBytes(const Bytes& bytes)
{
    if (bytes.size() == 0)
        return *this;

    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    flushIndexMarks();

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
    flushIndexMarks();
    findLastByte() |= 0x40;
    return *this;
}

Fluxmap& Fluxmap::appendDesync()
{
    appendByte(F_DESYNC);
    return *this;
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

const std::vector<nanoseconds_t>& Fluxmap::getIndexMarks() const
{
    std::scoped_lock lock(_mutationMutex);
    if (!_indexMarks.has_value())
    {
        _indexMarks = std::make_optional<std::vector<nanoseconds_t>>();
        FluxmapReader fmr(*this);
        for (;;)
        {
            unsigned ticks;
            if (!fmr.findEvent(F_BIT_INDEX, ticks))
                break;
            _indexMarks->push_back(fmr.tell().ns());
        }
    }
    return *_indexMarks;
}

void Fluxmap::flushIndexMarks()
{
    std::scoped_lock lock(_mutationMutex);
    _indexMarks = {};
}
