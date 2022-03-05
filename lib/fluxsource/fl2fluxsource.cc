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
    Fl2FluxSource(const Fl2FluxSourceProto& config): _config(config)
    {
        std::ifstream ifs(_config.filename(), std::ios::in | std::ios::binary);
        if (!ifs.is_open())
            Error() << fmt::format("cannot open input file '{}': {}",
                _config.filename(),
                strerror(errno));

        if (!_proto.ParseFromIstream(&ifs))
            Error() << "unable to read input file";
		upgradeFluxFile();
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int cylinder, int head)
    {
        for (const auto& track : _proto.track())
        {
            if ((track.cylinder() == cylinder) && (track.head() == head))
                return std::make_unique<Fluxmap>(track.flux());
        }

        return std::make_unique<Fluxmap>();
    }

    void recalibrate() {}

private:
    void check_for_error(std::ifstream& ifs)
    {
        if (ifs.fail())
            Error() << fmt::format("FL2 read I/O error: {}", strerror(errno));
    }

	void upgradeFluxFile()
	{
		if (_proto.version() == FluxFileVersion::VERSION_1)
		{
			/* Change a flux datastream with multiple segments separated by F_DESYNC into multiple
			 * flux segments. */

			for (auto* track = _proto.mutable_track())
			{
				Fluxmap old_flux(track.flux(0));
				auto split_flux = old_flux.split();

				track.clear_flux();
				for (const auto& flux : split_flux)
					track.add_flux(flux.rawBytes());
			}

			_proto.set_version(FluxFileVersion::VERSION_2);
		}
		if (_proto.version() > FluxFileVersion::VERSION_2)
			Error() << fmt::format("this is a version {} flux file, but this build of the client can only handle up to version {} --- please upgrade", _proto.version(), FluxFileVersion::VERSION_2);
	}

private:
    const Fl2FluxSourceProto& _config;
    FluxFileProto _proto;
};

std::unique_ptr<FluxSource> FluxSource::createFl2FluxSource(
    const Fl2FluxSourceProto& config)
{
    char buffer[16];
    std::ifstream(config.filename(), std::ios::in | std::ios::binary)
        .read(buffer, 16);
    if (strncmp(buffer, "SQLite format 3", 16) == 0)
        Error() << "this flux file is too old; please use the "
                   "upgrade-flux-file tool to upgrade it";

    return std::unique_ptr<FluxSource>(new Fl2FluxSource(config));
}
