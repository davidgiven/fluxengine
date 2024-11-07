#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"

class TestPatternFluxSource : public TrivialFluxSource
{
public:
    TestPatternFluxSource(const TestPatternFluxSourceProto& config):
        _config(config)
    {
    }

    ~TestPatternFluxSource() {}

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        auto fluxmap = std::make_unique<Fluxmap>();

        while (fluxmap->duration() < (_config.sequence_length_ms() * 1e6))
        {
            fluxmap->appendInterval(_config.interval_us() * TICKS_PER_US);
            fluxmap->appendPulse();
        }

        return fluxmap;
    }

    void recalibrate() override {}

private:
    const TestPatternFluxSourceProto& _config;
};

std::unique_ptr<FluxSource> FluxSource::createTestPatternFluxSource(
    const TestPatternFluxSourceProto& config)
{
    return std::make_unique<TestPatternFluxSource>(config);
}
