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

static FlagGroup flags { &writerFlags };

extern std::string writables_brother240_pb();
extern std::string writables_ibm1440_pb();

static std::map<std::string, std::function<std::string()>> writables = {
	{ "brother240", writables_brother240_pb },
	{ "ibm1440",    writables_ibm1440_pb },
};

int mainWrite(int argc, const char* argv[])
{
    std::vector<std::string> filenames = flags.parseFlagsWithFilenames(argc, argv);
	for (const auto& filename : filenames)
	{
		if (writables.find(filename) != writables.end())
		{
			if (!config.ParseFromString(writables[filename]()))
				Error() << "couldn't load config proto";
		}
		else
			Error() << "configs in files not supported yet";
	}

	if (!config.has_input() || !config.has_output())
		Error() << "incomplete config (did you remember to specify the format?)";

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	std::cout << s << '\n';

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.input().file()));
	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.output().disk()));

	writeDiskCommand(*reader, *encoder, *fluxSink);

    return 0;
}

