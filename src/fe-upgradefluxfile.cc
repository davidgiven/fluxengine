#include "globals.h"
#include "flags.h"
#include "sql.h"
#include "fluxmap.h"
#include "writer.h"
#include "proto.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "fmt/format.h"
#include <fstream>

int mainUpgradeFluxFile(int argc, const char* argv[])
{
    if (argc != 2)
        Error() << "syntax: fluxengine upgradefluxfile <fluxfile>";
    std::string filename = argv[1];
	std::string newfilename = filename + ".new";
    
    setRange(config.mutable_cylinders(), "0-79");
    setRange(config.mutable_heads(), "0-1");

	FluxSourceProto fluxSourceProto;
	fluxSourceProto.mutable_fl2()->set_filename(filename);

	FluxSinkProto fluxSinkProto;
	fluxSinkProto.mutable_fl2()->set_filename(newfilename);

	auto fluxSource = FluxSource::create(fluxSourceProto);
	auto fluxSink = FluxSink::create(fluxSinkProto);
	writeRawDiskCommand(*fluxSource, *fluxSink);

	rename(newfilename.c_str(), filename.c_str());
    return 0;
}
