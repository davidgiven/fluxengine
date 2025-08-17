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

        auto fields = findAllProtoFields(&f);
        std::ranges::sort(fields,
            [](const auto& o1, const auto& o2)
            {
                return doj::alphanum_comp(o1.path(), o2.path()) < 0;
            });

        for (const auto& pf : fields)
        {
            auto path = pf.path();
            if (pf.descriptor()->options().GetExtension(::isflux))
            {
                Fluxmap fluxmap(pf.getBytes());
                fmt::print("{}: {:0.3f} ms of flux in {} bytes\n",
                    path,
                    fluxmap.duration() / 1000000.0,
                    fluxmap.bytes());
            }
            else
                fmt::print("{}: {}\n", path, pf.get());
        }
    }

    return 0;
}
