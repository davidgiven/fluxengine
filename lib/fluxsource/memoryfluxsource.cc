#include "lib/globals.h"
#include "lib/fluxmap.h"
#include "lib/flux.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxmap.h"
#include <fmt/format.h>
#include <fstream>

class MemoryFluxSourceIterator : public FluxSourceIterator
{
public:
	MemoryFluxSourceIterator(const TrackFlux& track):
		_track(track)
	{}

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

class MemoryFluxSource : public FluxSource
{
public:
    MemoryFluxSource(const DiskFlux& flux): _flux(flux)
    {
    }

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int head) override
    {
        for (const auto& trackFlux : _flux.tracks)
        {
			if ((trackFlux->location.physicalTrack == track) && (trackFlux->location.head == head))
				return std::make_unique<MemoryFluxSourceIterator>(*trackFlux);
        }

        return std::make_unique<EmptyFluxSourceIterator>();
    }

    void recalibrate() {}

private:
	const DiskFlux& _flux;
};

std::unique_ptr<FluxSource> FluxSource::createMemoryFluxSource(const DiskFlux& flux)
{
	return std::make_unique<MemoryFluxSource>(flux);
}

