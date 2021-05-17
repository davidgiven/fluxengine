#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsource/fluxsource.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
#include "fmt/format.h"
#include <regex>

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxSource> FluxSource::create(const FluxSourceProto& config)
{
	switch (config.source_case())
	{
		case FluxSourceProto::kFluxfile:
			return createSqliteFluxSource(config.fluxfile());

		case FluxSourceProto::kDrive:
			return createHardwareFluxSource(config.drive());

		case FluxSourceProto::kErase:
			return createEraseFluxSource(config.erase());

		case FluxSourceProto::kKryoflux:
			return createKryofluxFluxSource(config.kryoflux());

		case FluxSourceProto::kTestPattern:
			return createTestPatternFluxSource(config.test_pattern());

		case FluxSourceProto::kScp:
			return createScpFluxSource(config.scp());
	}

	Error() << "bad input disk configuration";
    return std::unique_ptr<FluxSource>();
}

void FluxSource::updateConfigForFilename(const std::string& filename)
{
	FluxSourceProto* f = config.mutable_input()->mutable_flux();
	static const std::vector<std::pair<std::regex, std::function<void(const std::string&)>>> formats =
	{
		{ std::regex("^(.*\\.flux)$"),     [&](const auto& s) { f->set_fluxfile(s); }},
		{ std::regex("^(.*\\.scp)$"),      [&](const auto& s) { f->mutable_scp()->set_filename(s); }},
		{ std::regex("^erase:$"),          [&](const auto& s) { f->mutable_erase(); }},
		{ std::regex("^kryoflux:(.*)$"),   [&](const auto& s) { f->mutable_kryoflux()->set_directory(s); }},
		{ std::regex("^testpattern:(.*)"), [&](const auto& s) { f->mutable_test_pattern(); }},
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



