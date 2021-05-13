#include "globals.h"
#include "fluxmap.h"
#include "fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

class TestPatternFluxSource : public FluxSource
{
public:
    TestPatternFluxSource(const TestPatternInput& config):
		_config(config)
    {}

    ~TestPatternFluxSource() {}

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
		std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);

		while (fluxmap->duration() < (_config.sequence_length_us()*1000000.0))
		{
			fluxmap->appendInterval(_config.interval_us());
			fluxmap->appendPulse();
		}

		return fluxmap;
    }

    void recalibrate() {}

private:
	const TestPatternInput& _config;
};

std::unique_ptr<FluxSource> FluxSource::createTestPatternFluxSource(const TestPatternInput& config)
{
    return std::unique_ptr<FluxSource>(new TestPatternFluxSource(config));
}


