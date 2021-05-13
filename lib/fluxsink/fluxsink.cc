#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "lib/config.pb.h"

std::unique_ptr<FluxSink> FluxSink::create(const OutputDiskProto& config)
{
	switch (config.dest_case())
	{
		case OutputDiskProto::kFluxfile:
			return createSqliteFluxSink(config.fluxfile());

		case OutputDiskProto::kDrive:
			return createHardwareFluxSink(config.drive());
	}

	Error() << "bad output disk config";
	return std::unique_ptr<FluxSink>();
}
