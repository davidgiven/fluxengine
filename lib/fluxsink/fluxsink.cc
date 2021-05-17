#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
#include "fmt/format.h"
#include <regex>

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
	static const std::vector<std::pair<std::regex, std::function<void(const std::string&)>>> formats =
	{
		{ std::regex("^(.*\\.flux)$"), [&](const auto& s) { f->set_fluxfile(s); }},
		{ std::regex("^(.*\\.scp)$"),  [&](const auto& s) { f->mutable_scp()->set_filename(s); }},
		{ std::regex("^vcd:(.*)$"),    [&](const auto& s) { f->mutable_vcd()->set_directory(s); }},
		{ std::regex("^au:(.*)$"),     [&](const auto& s) { f->mutable_au()->set_directory(s); }},
	};

	for (const auto& it : formats)
	{
		std::smatch match;
		if (std::regex_match(filename, match, it.first))
		{
			it.second(match[1]);
			return;
		}
	}

	Error() << fmt::format("unrecognised flux filename '{}'", filename);
}


