#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsource/fluxsource.h"
#include "lib/config.pb.h"

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxSource> FluxSource::create(const FluxSpec& spec)
{
    const auto& filename = spec.filename;

    if (filename.empty())
        return createHardwareFluxSource(spec.drive);
    else if (ends_with(filename, ".flux"))
        return createSqliteFluxSource(filename);
    else if (ends_with(filename, "/"))
        return createStreamFluxSource(filename);

    Error() << "unrecognised flux filename extension";
    return std::unique_ptr<FluxSource>();
}

std::unique_ptr<FluxSource> FluxSource::create(const Config_InputDisk& config)
{
	if (config.has_fluxfile())
		return createSqliteFluxSource(config.fluxfile());
	else
		return createHardwareFluxSource(config.drive());

	Error() << "bad input disk configuration";
    return std::unique_ptr<FluxSource>();
}

