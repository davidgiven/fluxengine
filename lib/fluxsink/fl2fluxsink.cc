#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sql.h"
#include "bytes.h"
#include "protocol.h"
#include "fluxsink/fluxsink.h"
#include "decoders/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "proto.h"
#include "fmt/format.h"
#include "lib/fl2.pb.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

class Fl2FluxSink : public FluxSink
{
public:
	Fl2FluxSink(const Fl2FluxSinkProto& lconfig):
		_config(lconfig),
		_of(lconfig.filename())
	{
		if (!_of.is_open())
			Error() << "cannot open output file";
		_proto.set_version(FluxFileVersion::VERSION_1);
	}

	~Fl2FluxSink()
	{
		std::cerr << "FL2: writing output file\n";
		if (!_proto.SerializeToOstream(&_of))
			Error() << "unable to write output file";
		_of.close();
	}

public:
	void writeFlux(int cylinder, int head, Fluxmap& fluxmap)
	{
		auto track = _proto.add_track();
		track->set_cylinder(cylinder);
		track->set_head(head);
		track->set_flux(fluxmap.rawBytes());
	}

	operator std::string () const
	{
		return fmt::format("fl2({})", _config.filename());
	}

private:
	const Fl2FluxSinkProto& _config;
	std::ofstream _of;
	FluxFileProto _proto;
};

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(const Fl2FluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(config));
}


