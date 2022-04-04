#include "globals.h"
#include "fluxmap.h"
#include "fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

class TestPatternFluxSource : public TrivialFluxSource
{
public:
    TestPatternFluxSource(const TestPatternFluxSourceProto& config):
		_config(config)
    {}

    ~TestPatternFluxSource() {}

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
		auto fluxmap = std::make_unique<Fluxmap>();

		while (fluxmap->duration() < (_config.sequence_length_us()*1000000.0))
		{
			fluxmap->appendInterval(_config.interval_us());
			fluxmap->appendPulse();
		}

		return fluxmap;
    }

    void recalibrate() {}

private:
	const TestPatternFluxSourceProto& _config;
};

std::unique_ptr<FluxSource> FluxSource::createTestPatternFluxSource(const TestPatternFluxSourceProto& config)
{
    return std::make_unique<TestPatternFluxSource>(config);
}


