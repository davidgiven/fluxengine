#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/config/proto.h"
#include <fstream>

struct A2Rv2Flux
{
    std::vector<Bytes> flux;
    nanoseconds_t index;
};

class A2rv2FluxSourceIterator : public FluxSourceIterator
{
public:
    A2rv2FluxSourceIterator(A2Rv2Flux& flux): _flux(flux) {}

    bool hasNext() const override
    {
        return _count != _flux.flux.size();
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        nanoseconds_t index = _flux.index;
        Bytes& asbytes = _flux.flux[_count++];
        ByteReader br(asbytes);

        auto fluxmap = std::make_unique<Fluxmap>();
        while (!br.eof())
        {
            unsigned aticks = 0;
            for (;;)
            {
                unsigned i = br.read_8();
                aticks += i;
                if (i != 0xff)
                    break;
            }

            nanoseconds_t interval = aticks * 125;
            if ((index >= 0) && (index < interval))
            {
                fluxmap->appendInterval(index);
                fluxmap->appendIndex();
                interval -= index;
            }
            index -= interval;

            fluxmap->appendInterval(interval / NS_PER_TICK);
            fluxmap->appendPulse();
        }

        return fluxmap;
    }

private:
    A2Rv2Flux& _flux;
    int _count = 0;
};

class A2rFluxSource : public FluxSource
{
public:
    A2rFluxSource(const A2rFluxSourceProto& config): _config(config)
    {
        _data = Bytes::readFromFile(_config.filename());
        ByteReader br(_data);

        switch (br.read_be32())
        {
            case 0x41325232:
            {
                _version = 2;
                Bytes info = findChunk("INFO");
                int disktype = info[33];
                if (disktype == 1)
                {
                    /* 5.25" with quarter stepping. */
                    _extraConfig.mutable_drive()->set_tracks(160);
                    _extraConfig.mutable_drive()->set_heads(1);
                    _extraConfig.mutable_drive()->set_drive_type(
                        DRIVETYPE_APPLE2);
                }
                else
                {
                    /* 3.5". */
                    _extraConfig.mutable_drive()->set_drive_type(
                        DRIVETYPE_80TRACK);
                }

                Bytes stream = findChunk("STRM");
                ByteReader bsr(stream);
                for (;;)
                {
                    int location = bsr.read_8();
                    if (location == 0xff)
                        break;
                    auto key = (disktype == 1) ? std::make_pair(location, 0)
                                               : std::make_pair(location >> 1,
                                                     location & 1);

                    bsr.skip(1);
                    uint32_t len = bsr.read_le32();
                    nanoseconds_t index = (nanoseconds_t)bsr.read_le32() * 125;
                    auto it = _v2data.find(key);
                    if (it == _v2data.end())
                    {
                        _v2data[key] = std::make_unique<A2Rv2Flux>();
                        it = _v2data.find(key);
                        it->second->index = index;
                    }

                    it->second->flux.push_back(bsr.read(len));
                }

                break;
            }

            default:
                error("unsupported A2R version");
        }
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int head) override
    {
        switch (_version)
        {
            case 2:
            {
                auto i = _v2data.find(std::make_pair(track, head));
                if (i != _v2data.end())
                    return std::make_unique<A2rv2FluxSourceIterator>(
                        *i->second);
                else
                    return std::make_unique<EmptyFluxSourceIterator>();
            }

            default:
                error("unsupported A2R version");
        }
    }

    void recalibrate() override {}

private:
    Bytes findChunk(Bytes id)
    {
        uint32_t offset = 8;
        while (offset < _data.size())
        {
            ByteReader br(_data);
            br.seek(offset);
            if (br.read(4) == id)
            {
                uint32_t size = br.read_le32();
                return br.read(size);
            }

            offset += br.read_le32() + 8;
        }

        error("A2R file missing chunk");
    }

private:
    const A2rFluxSourceProto& _config;
    Bytes _data;
    std::ifstream _if;
    int _version;
    std::map<std::pair<int, int>, std::unique_ptr<A2Rv2Flux>> _v2data;
};

std::unique_ptr<FluxSource> FluxSource::createA2rFluxSource(
    const A2rFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new A2rFluxSource(config));
}
