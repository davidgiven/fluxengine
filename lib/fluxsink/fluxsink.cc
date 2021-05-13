#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"

std::unique_ptr<FluxSink> FluxSink::create(const Config_OutputDisk& config)
{
	switch (config.dest_case())
	{
		case Config_OutputDisk::kFluxfile:
			return createSqliteFluxSink(config.fluxfile());

		case Config_OutputDisk::kDrive:
			return createHardwareFluxSink(config.drive());
	}

	Error() << "bad output disk config";
	return std::unique_ptr<FluxSink>();
}
