#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
#include "fmt/format.h"

std::unique_ptr<FluxSink> FluxSink::create(const FluxSinkProto& config)
{
	switch (config.dest_case())
	{
		case FluxSinkProto::kFluxfile:
			return createSqliteFluxSink(config.fluxfile());

		case FluxSinkProto::kDrive:
			return createHardwareFluxSink(config.drive());

		case FluxSinkProto::kAu:
			return createAuFluxSink(config.au());

		case FluxSinkProto::kVcd:
			return createVcdFluxSink(config.vcd());

		case FluxSinkProto::kScp:
			return createScpFluxSink(config.scp());
	}

	Error() << "bad output disk config";
	return std::unique_ptr<FluxSink>();
}

void FluxSink::updateConfigForFilename(const std::string& filename)
{
	FluxSinkProto* f = config.mutable_output()->mutable_flux();
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".flux",     [&]() { f->set_fluxfile(filename); }},
		{".scp",      [&]() { f->mutable_scp()->set_filename(filename); }},
		{".vcd",      [&]() { f->mutable_vcd()->set_directory(filename); }},
		{".au",       [&]() { f->mutable_au()->set_directory(filename); }},
	};

	for (const auto& it : formats)
	{
		if (endsWith(filename, it.first))
		{
			it.second();
			return;
		}
	}

	Error() << fmt::format("unrecognised flux filename '{}'", filename);
}


