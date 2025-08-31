extern "C" void forceLinkPlugin_fonts();
extern "C" void forceLinkPlugin_ui();
extern "C" void forceLinkPlugin_builtin();

namespace
{
    struct StaticLoad
    {
        StaticLoad()
        {
            // forceLinkPlugin_fluxengine();
            forceLinkPlugin_fonts();
            forceLinkPlugin_ui();
            forceLinkPlugin_builtin();
        }
    };
}

static StaticLoad staticLoad;
