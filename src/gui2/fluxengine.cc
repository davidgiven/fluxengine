#include <hex/plugin.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <romfs/romfs.hpp>
#include "customview.h"
#include "configview.h"
#include "summaryview.h"
#include "diskprovider.h"
#include "datastore.h"

IMHEX_PLUGIN_SETUP("FluxEngine", "David Given", "FluxEngine integration")
{
    hex::log::debug("Using romfs: '{}'", romfs::name());
    hex::LocalizationManager::addLanguages(
        romfs::get("lang/languages.json").string(),
        [](const std::filesystem::path& path)
        {
            return romfs::get(path).string();
        });

    hex::ContentRegistry::Provider::add<DiskProvider>();

    hex::ContentRegistry::Views::add<CustomView>();
    hex::ContentRegistry::Views::add<ConfigView>();
    hex::ContentRegistry::Views::add<SummaryView>();

    Datastore::init();
}
