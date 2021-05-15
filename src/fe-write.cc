#include "globals.h"
#include "flags.h"
#include "writer.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "proto.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

extern const std::map<std::string, std::string> writables;

int mainWrite(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, writables);

	if (!config.has_input() || !config.has_output())
		Error() << "incomplete config (did you remember to specify the format?)";

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.input().file()));
	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.output().disk()));

	writeDiskCommand(*reader, *encoder, *fluxSink);

    return 0;
}

