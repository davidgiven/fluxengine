#include "globals.h"
#include "fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fluxsource/fluxsource.h"
#include <fstream>

class FlxFluxSource : public TrivialFluxSource
{
public:
    FlxFluxSource(const FlxFluxSourceProto& config): _path(config.directory())
    {
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        std::string path = fmt::format("{}/@TR{:02}S{}.FLX", _path, track, side + 1);

		std::ifstream is(path);
        if (!is.good())
            Error() << fmt::format("cannot access path '{}'", _path);

        return convertStream(is);
    }

    void recalibrate() {}

private:
	std::unique_ptr<const Fluxmap> convertStream(std::ifstream& is)
	{
		/* Skip header. */

		for (;;)
		{
			int c = is.get();
			if (!c)
				break;
			if (is.eof())
				Error() << fmt::format("malformed FLX stream");
		}

		return nullptr;
	}

private:
    const std::string _path;
};

std::unique_ptr<FluxSource> FluxSource::createFlxFluxSource(
    const FlxFluxSourceProto& config)
{
    return std::make_unique<FlxFluxSource>(config);
}
