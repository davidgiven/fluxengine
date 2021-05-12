#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "macintosh/macintosh.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "proto.h"
#include "dataspec.h"
#include "fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags { &readerFlags };

extern std::string readables_brother_pb();
extern std::string readables_ibm_pb();

static std::map<std::string, std::function<std::string()>> readables = {
	{ "brother", readables_brother_pb },
	{ "ibm",     readables_ibm_pb },
};

int mainRead(int argc, const char* argv[])
{
    std::vector<std::string> filenames = flags.parseFlagsWithFilenames(argc, argv);
	for (const auto& filename : filenames)
	{
		if (readables.find(filename) != readables.end())
		{
			if (!config.ParseFromString(readables[filename]()))
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

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.input().disk()));

	const Config_InputDisk& disk = config.input().disk();
	std::unique_ptr<AbstractDecoder> decoder;
	if (disk.has_brother())
		decoder.reset(new BrotherDecoder(disk.brother()));
	else if (disk.has_ibm())
		decoder.reset(new IbmDecoder(disk.ibm()));
	else
		Error() << "no input disk format specified";

	std::unique_ptr<ImageWriter> writer(ImageWriter::create(config.output().file()));

	readDiskCommand(*fluxSource, *decoder, *writer);

    return 0;
}

