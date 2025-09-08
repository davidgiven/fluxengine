#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/flux.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/data/fluxmap.h"
#include "lib/data/layout.h"
#include <fstream>

class MemoryFluxSourceIterator : public FluxSourceIterator
{
public:
    MemoryFluxSourceIterator(const TrackFlux& track): _track(track) {}

    bool hasNext() const override
    {
        return _count < _track.trackDatas.size();
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        auto bytes = _track.trackDatas[_count]->fluxmap->rawBytes();
        _count++;
        return std::make_unique<Fluxmap>(bytes);
    }

private:
    const TrackFlux& _track;
    int _count = 0;
};

class MemoryFluxSource : public FluxSource
{
public:
    MemoryFluxSource(const DiskFlux& flux): _flux(flux)
    {
        std::vector<CylinderHead> chs;
        for (const auto& trackFlux : flux.tracks)
            chs.push_back(
                CylinderHead{(unsigned)trackFlux->trackInfo->physicalCylinder,
                    (unsigned)trackFlux->trackInfo->logicalCylinder});
        _extraConfig.mutable_drive()->set_tracks(
            convertCylinderHeadsToString(chs));
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(
        int physicalCylinder, int physicalHead) override
    {
        for (const auto& trackFlux : _flux.tracks)
        {
            if ((trackFlux->trackInfo->physicalCylinder == physicalCylinder) &&
                (trackFlux->trackInfo->physicalHead == physicalHead))
                return std::make_unique<MemoryFluxSourceIterator>(*trackFlux);
        }

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
