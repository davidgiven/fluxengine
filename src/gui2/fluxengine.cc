#include <hex/plugin.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/workspace_manager.hpp>
#include <hex/helpers/default_paths.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <romfs/romfs.hpp>
#include "globals.h"
#include "imageview.h"
#include "physicalview.h"
#include "summaryview.h"
#include "configview.h"
#include "controlpanelview.h"
#include "logview.h"
#include "visualiserview.h"
#include "exerciserview.h"
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

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"hex.builtin.menu.workspace", "fluxengine.menu.workspace.reset"},
        ICON_TA_CANCEL,
        10000,
        hex::Shortcut::None,
        []
        {
            static const std::string extractFolder = "auto_extract/workspaces";
            for (const auto& romfsPath : romfs::list(extractFolder))
            {
                for (const auto& imhexPath : hex::paths::getDataPaths(false))
                {
                    const auto path =
                        imhexPath / std::fs::relative(romfsPath, extractFolder);
                    hex::log::info("Extracting {} to {}",
                        romfsPath.string(),
                        path.string());

                    wolv::io::File file(path, wolv::io::File::Mode::Create);
                    if (!file.isValid())
                        continue;

                    auto data = romfs::get(romfsPath).span<u8>();
                    file.writeBuffer(data.data(), data.size());

                    if (file.getSize() == data.size())
                        break;
                }
            }

            auto currentWorkspaceName =
                hex::WorkspaceManager::getCurrentWorkspace()->first;
            hex::WorkspaceManager::reload();
            hex::WorkspaceManager::switchWorkspace(currentWorkspaceName);
        });

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"hex.builtin.menu.extras", "fluxengine.menu.tools.exerciser"},
        ICON_TA_TOOLS,
        2500,
        hex::Shortcut::None,
        []
        {
            hex::ContentRegistry::Views::getViewByName(
                "fluxengine.view.exerciser.name")
                ->getWindowOpenState() = true;
        });

    hex::ContentRegistry::Provider::add<DiskProvider>();

    hex::ContentRegistry::Views::add<ConfigView>();
    hex::ContentRegistry::Views::add<ControlPanelView>();
    hex::ContentRegistry::Views::add<ImageView>();
    hex::ContentRegistry::Views::add<LogView>();
    hex::ContentRegistry::Views::add<PhysicalView>();
    hex::ContentRegistry::Views::add<SummaryView>();
    hex::ContentRegistry::Views::add<VisualiserView>();
    hex::ContentRegistry::Views::add<ExerciserView>();

    Datastore::init();
}
