#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/data/flux.h"
#include "lib/external/fl2.h"
#include "lib/external/fl2.pb.h"
#include "dep/alphanum/alphanum.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;
static std::string filename;

int mainFluxfileLs(int argc, const char* argv[])
{
    const auto filenames = flags.parseFlagsWithFilenames(argc, argv);
    if (filenames.size() != 1)
        error("you must specify exactly one filename");

    for (const auto& filename : filenames)
    {
        fmt::print("Contents of {}:\n", filename);
        FluxFileProto f = loadFl2File(filename);

        auto fields = findAllProtoFields(f);
		std::set<std::string, doj::alphanum_less<std::string>> fieldsSorted;
        for (const auto& e : fields)
			fieldsSorted.insert(e.first);

        for (const auto& e : fieldsSorted)
        {
            fmt::print("{}: {}\n", e, getProtoByString(&f, e));
        }
    }

    return 0;
}
