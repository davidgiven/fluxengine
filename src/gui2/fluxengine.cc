#include <hex/plugin.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/workspace_manager.hpp>
#include <hex/helpers/default_paths.hpp>
#include <hex/helpers/fs.hpp>
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

    hex::ContentRegistry::UserInterface::registerMainMenuItem(
        "fluxengine.menu.name", 4999);

    auto isReady = []
    {
        return !Datastore::isBusy();
    };

    auto isReadyAndHasImage = []
    {
        return !Datastore::isBusy() && Datastore::getDisk()->image;
    };

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.readDevice"},
        ICON_TA_DEVICE_FLOPPY,
        1000,
        hex::Shortcut::None,
        []
        {
            Datastore::beginRead(false);
        },
        isReady);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.rereadBad"},
        ICON_TA_REPEAT,
        1100,
        hex::Shortcut::None,
        []
        {
            Datastore::beginRead(true);
        },
        isReadyAndHasImage);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.readImage"},
        ICON_VS_FOLDER_OPENED,
        1200,
        hex::Shortcut::None,
        []
        {
            hex::fs::openFileBrowser(
                hex::fs::DialogMode::Open, {}, Datastore::readImage);
        },
        isReady);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.createBlank"},
        ICON_VS_FOLDER_OPENED,
        1250,
        hex::Shortcut::None,
        Datastore::createBlankImage,
        []
        {
            return !Datastore::isBusy() && Datastore::canFormat();
        });

    hex::ContentRegistry::UserInterface::addMenuItemSeparator(
        {"fluxengine.menu.name"}, 1300);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.writeDevice"},
        ICON_TA_DEVICE_FLOPPY,
        1400,
        hex::Shortcut::None,
        Datastore::beginWrite,
        isReadyAndHasImage);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.writeFlux"},
        ICON_TA_DOWNLOAD,
        1500,
        hex::Shortcut::None,
        []
        {
            hex::fs::openFileBrowser(
                hex::fs::DialogMode::Save, {}, Datastore::writeFluxFile);
        },
        isReady);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.view.controlpanel.writeImage"},
        ICON_VS_SAVE_ALL,
        1600,
        hex::Shortcut::None,
        []
        {
            hex::fs::openFileBrowser(
                hex::fs::DialogMode::Save, {}, Datastore::writeImage);
        },
        isReadyAndHasImage);

    hex::ContentRegistry::UserInterface::addMenuItemSeparator(
        {"fluxengine.menu.name"}, 9999);

    hex::ContentRegistry::UserInterface::addMenuItem(
        {"fluxengine.menu.name", "fluxengine.menu.disk.exerciser"},
        ICON_TA_TOOLS,
        10000,
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
