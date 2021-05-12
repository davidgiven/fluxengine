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
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags { &readerFlags };

extern const char readables_brother_pb[];
extern const int readables_brother_pb_size;

int mainRead(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

	if (!config.ParseFromString(std::string(readables_brother_pb, readables_brother_pb_size)))
		Error() << "couldn't load config proto";

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	std::cout << s << '\n';

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.input().disk()));

	std::unique_ptr<AbstractDecoder> decoder;
	if (config.input().disk().has_brother())
		decoder.reset(new BrotherDecoder());
	else
		Error() << "no input disk format specified";

	std::unique_ptr<ImageWriter> writer(ImageWriter::create(config.output().file()));

	readDiskCommand(*fluxSource, *decoder, *writer);

    return 0;
}

