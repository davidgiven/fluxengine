#include "globals.h"
#include "fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fl2.pb.h"
#include "fluxsource/fluxsource.h"
#include "proto.h"
#include "fl2.h"
#include "fmt/format.h"
#include "fluxmap.h"
#include <fstream>

class Fl2FluxSourceIterator : public FluxSourceIterator
{
public:
    Fl2FluxSourceIterator(const TrackFluxProto& proto): _proto(proto) {}

    bool hasNext() const override
    {
        return _count < _proto.flux_size();
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        auto bytes = _proto.flux(_count);
        _count++;
        return std::make_unique<Fluxmap>(bytes);
    }

private:
    const TrackFluxProto& _proto;
    int _count = 0;
};

class EmptyFluxSourceIterator : public FluxSourceIterator
{
    bool hasNext() const override
    {
        return false;
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        Error() << "no flux to read";
    }
};

class Fl2FluxSource : public FluxSource
{
public:
    Fl2FluxSource(const Fl2FluxSourceProto& config): _config(config)
    {
        _proto = loadFl2File(_config.filename());
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int head) override
    {
        for (const auto& trackFlux : _proto.track())
        {
            if ((trackFlux.track() == track) && (trackFlux.head() == head))
                return std::make_unique<Fl2FluxSourceIterator>(trackFlux);
        }

        return std::make_unique<EmptyFluxSourceIterator>();
    }

    void recalibrate() {}

private:
    void check_for_error(std::ifstream& ifs)
    {
        if (ifs.fail())
            Error() << fmt::format("FL2 read I/O error: {}", strerror(errno));
    }

private:
    const Fl2FluxSourceProto& _config;
    FluxFileProto _proto;
};

std::unique_ptr<FluxSource> FluxSource::createFl2FluxSource(
    const Fl2FluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new Fl2FluxSource(config));
}
