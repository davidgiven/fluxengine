#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/flux.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/data/fluxmap.h"
#include "lib/data/layout.h"
#include <fstream>

class MemoryFluxSourceIterator : public FluxSourceIterator
{
    using multimap =
        std::multimap<CylinderHead, std::shared_ptr<const TrackDataFlux>>;

public:
    MemoryFluxSourceIterator(
        multimap::const_iterator startIt, multimap::const_iterator endIt):
        _startIt(startIt),
        _endIt(endIt)
    {
    }

    bool hasNext() const override
    {
        return _startIt != _endIt;
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        auto bytes = _startIt->second->fluxmap->rawBytes();
        _startIt++;
        return std::make_unique<Fluxmap>(bytes);
    }

private:
    multimap::const_iterator _startIt;
    multimap::const_iterator _endIt;
};

class MemoryFluxSource : public FluxSource
{
public:
    MemoryFluxSource(const DiskFlux& flux): _flux(flux)
    {
        std::set<CylinderHead> chs;
        for (auto& [ch, trackDataFlux] : flux.fluxesByTrack)
            chs.insert(ch);
        _extraConfig.mutable_drive()->set_tracks(
            convertCylinderHeadsToString(std::vector(chs.begin(), chs.end())));
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(
        int physicalCylinder, int physicalHead) override
    {
        auto [startIt, endIt] = _flux.fluxesByTrack.equal_range(
            {(unsigned)physicalCylinder, (unsigned)physicalHead});
        if (startIt != _flux.fluxesByTrack.end())
            return std::make_unique<MemoryFluxSourceIterator>(startIt, endIt);

        return std::make_unique<EmptyFluxSourceIterator>();
    }

    void recalibrate() override {}

private:
    const DiskFlux& _flux;
};

std::unique_ptr<FluxSource> FluxSource::createMemoryFluxSource(
    const DiskFlux& flux)
{
    return std::make_unique<MemoryFluxSource>(flux);
}
