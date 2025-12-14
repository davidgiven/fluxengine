#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static void ignoreInapplicableValueExceptions(std::function<void(void)> cb)
{
    try
    {
        cb();
    }
    catch (const InapplicableValueException* e)
    {
        /* swallow */
    }
}

FlagGroup fileFlags;

static StringFlag image({"-i", "--image"},
    "image to work on",
    "",
    [](const auto& value)
    {
        ignoreInapplicableValueExceptions(
            [&]()
            {
                globalConfig().setImageReader(value);
            });
        ignoreInapplicableValueExceptions(
            [&]()
            {
                globalConfig().setImageWriter(value);
            });
    });

static StringFlag flux({"-f", "--flux"},
    "flux source to work on",
    "",
    [](const auto& value)
    {
        ignoreInapplicableValueExceptions(
            [&]()
            {
                globalConfig().setFluxSink(value);
            });
        ignoreInapplicableValueExceptions(
            [&]()
            {
                globalConfig().setFluxSource(value);
            });
    });
