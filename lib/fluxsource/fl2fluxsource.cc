#include "globals.h"
#include "fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fl2.pb.h"
#include "fluxsource/fluxsource.h"
#include "proto.h"
#include "fmt/format.h"
#include <fstream>

class Fl2FluxSource : public FluxSource
{
public:
    Fl2FluxSource(const Fl2FluxSourceProto& config):
		_config(config)
    {
		std::ifstream ifs(_config.filename(), std::ios::in | std::ios::binary);
		if (!ifs.is_open())
			Error() << fmt::format("cannot open input file '{}': {}",
				_config.filename(), strerror(errno));

		if (!_proto.ParseFromIstream(&ifs))
			Error() << "unable to read input file";
	}

public:
    std::unique_ptr<Fluxmap> readFlux(int cylinder, int head)
    {
		for (const auto& track : _proto.track())
		{
			if ((track.cylinder() == cylinder) && (track.head() == head))
				return std::make_unique<Fluxmap>(track.flux());
		}

		return std::unique_ptr<Fluxmap>();
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

std::unique_ptr<FluxSource> FluxSource::createFl2FluxSource(const Fl2FluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new Fl2FluxSource(config));
}

