#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"

std::unique_ptr<FluxSink> FluxSink::create(const Config_OutputDisk& config)
{
	if (config.has_fluxfile())
		return createSqliteFluxSink(config.fluxfile());
	return createHardwareFluxSink(config.drive());
}
