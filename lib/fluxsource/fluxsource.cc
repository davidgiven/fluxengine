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

std::unique_ptr<FluxSource> FluxSource::create(const Config_InputDisk& config)
{
	switch (config.source_case())
	{
		case Config_InputDisk::kFluxfile:
			return createSqliteFluxSource(config.fluxfile());

		case Config_InputDisk::kDrive:
			return createHardwareFluxSource(config.drive());

		case Config_InputDisk::kTestPattern:
			return createTestPatternFluxSource(config.test_pattern());
	}

	Error() << "bad input disk configuration";
    return std::unique_ptr<FluxSource>();
}

