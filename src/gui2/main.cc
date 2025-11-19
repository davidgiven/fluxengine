extern "C" void forceLinkPlugin_fonts();
extern "C" void forceLinkPlugin_ui();
extern "C" void forceLinkPlugin_builtin();
extern "C" void forceLinkPlugin_fluxengine();

namespace
{
    struct StaticLoad
    {
        StaticLoad()
        {
            forceLinkPlugin_fonts();
            forceLinkPlugin_ui();
            forceLinkPlugin_builtin();
            forceLinkPlugin_fluxengine();
        }
    };
}

static StaticLoad staticLoad;
