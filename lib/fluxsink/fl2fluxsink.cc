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
		_of(lconfig.filename(), std::ios::out | std::ios::binary)
	{
		if (!_of.is_open())
			Error() << "cannot open output file";
	}

	~Fl2FluxSink()
	{
		FluxFileProto proto;
		proto.set_version(FluxFileVersion::VERSION_1);
		for (const auto& e : _data)
		{
			auto track = proto.add_track();
			track->set_cylinder(e.first.first);
			track->set_head(e.first.second);
			track->set_flux(e.second);
		}

		if (!proto.SerializeToOstream(&_of))
			Error() << "unable to write output file";
		_of.close();
		if (_of.fail())
			Error() << "FL2 write I/O error: " << strerror(errno);
	}

public:
	void writeFlux(int cylinder, int head, Fluxmap& fluxmap)
	{
		_data[std::make_pair(cylinder, head)] = fluxmap.rawBytes();
	}

	operator std::string () const
	{
		return fmt::format("fl2({})", _config.filename());
	}

private:
	const Fl2FluxSinkProto& _config;
	std::ofstream _of;
	std::map<std::pair<unsigned, unsigned>, Bytes> _data;
};

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(const Fl2FluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(config));
}


