#include <hex/plugin.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/task_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/helpers/debugging.hpp>
#include <hex/helpers/utils.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/events/events_gui.hpp>
#include <hex/api/events/requests_gui.hpp>
#include <hex/ui/imgui_imhex_extensions.h>
#include <romfs/romfs.hpp>

#include "content/views/view_hex_editor.hpp"
#include "content/views/view_about.hpp"
#include "content/views/view_settings.hpp"
#include "content/views/view_theme_manager.hpp"
#include "content/views/view_logs.hpp"
#include "content/providers/null_provider.hpp"

#include <hex/api/layout_manager.hpp>
#include "customview.h"
#include "configview.h"
#include "summaryview.h"

// #include "content/command_line_interface.hpp"
// #include <banners/banner_icon.hpp>
// #include <fonts/vscode_icons.hpp>
#include <fmt/format.h>

namespace hex::plugin::builtin
{

    void registerEventHandlers();
    void registerDataVisualizers();
    void registerMiniMapVisualizers();
    void registerDataInspectorEntries();
    void registerToolEntries();
    void registerPatternLanguageFunctions();
    void registerPatternLanguageTypes();
    void registerPatternLanguagePragmas();
    void registerPatternLanguageVisualizers();
    void registerCommandPaletteCommands();
    void registerSettings();
    void loadSettings();
    void registerDataProcessorNodes();
    void registerProviders();
    void registerDataFormatters();
    void registerMainMenuEntries();
    void createWelcomeScreen();
    void registerThemeHandlers();
    void registerStyleHandlers();
    void registerThemes();
    void registerBackgroundServices();
    void registerNetworkEndpoints();
    void registerFileHandlers();
    void registerProjectHandlers();
    void registerAchievements();
    void registerReportGenerators();
    void registerTutorials();
    void registerDataInformationSections();
    void loadWorkspaces();

    void addWindowDecoration();
    void addFooterItems();
    void addTitleBarButtons();
    void addToolbarItems();
    void addGlobalUIItems();
    void addInitTasks();

    void handleBorderlessWindowMode();
    void setupOutOfBoxExperience();

    void extractBundledFiles();
}

namespace
{
    using namespace hex;

    bool configureUIScale()
    {
        EventDPIChanged::subscribe(
            [](float, float newScaling)
            {
                int interfaceScaleSetting =
                    int(ContentRegistry::Settings::read<float>(
                            "hex.builtin.setting.interface",
                            "hex.builtin.setting.interface.scaling_factor",
                            0.0F) *
                        10.0F);

                float interfaceScaling;
                if (interfaceScaleSetting == 0)
                    interfaceScaling = newScaling;
                else
                    interfaceScaling = interfaceScaleSetting / 10.0F;

                ImHexApi::System::impl::setGlobalScale(interfaceScaling);
            });

        const auto nativeScale = ImHexApi::System::getNativeScale();
        EventDPIChanged::post(nativeScale, nativeScale);

        return true;
    }

    bool loadWindowSettings()
    {
        bool multiWindowEnabled = ContentRegistry::Settings::read<bool>(
            "hex.builtin.setting.interface",
            "hex.builtin.setting.interface.multi_windows",
            false);
        ImHexApi::System::impl::setMultiWindowMode(multiWindowEnabled);

        bool restoreWindowPos = ContentRegistry::Settings::read<bool>(
            "hex.builtin.setting.interface",
            "hex.builtin.setting.interface.restore_window_pos",
            false);

        if (restoreWindowPos)
        {
            ImHexApi::System::InitialWindowProperties properties = {};

            properties.maximized = ContentRegistry::Settings::read<bool>(
                "hex.builtin.setting.interface",
                "hex.builtin.setting.interface.window.maximized",
                0);
            properties.x = ContentRegistry::Settings::read<int>(
                "hex.builtin.setting.interface",
                "hex.builtin.setting.interface.window.x",
                0);
            properties.y = ContentRegistry::Settings::read<int>(
                "hex.builtin.setting.interface",
                "hex.builtin.setting.interface.window.y",
                0);
            properties.width = ContentRegistry::Settings::read<int>(
                "hex.builtin.setting.interface",
                "hex.builtin.setting.interface.window.width",
                0);
            properties.height = ContentRegistry::Settings::read<int>(
                "hex.builtin.setting.interface",
                "hex.builtin.setting.interface.window.height",
                0);

            ImHexApi::System::impl::setInitialWindowProperties(properties);
        }

        return true;
    }

    void registerViews()
    {
        using namespace hex::plugin::builtin;

        ContentRegistry::Views::add<ViewHexEditor>();
        ContentRegistry::Views::add<ViewAbout>();
        ContentRegistry::Views::add<ViewSettings>();
        ContentRegistry::Views::add<ViewThemeManager>();
        ContentRegistry::Views::add<ViewLogs>();

        ContentRegistry::Views::add<CustomView>();
        ContentRegistry::Views::add<ConfigView>();
        ContentRegistry::Views::add<SummaryView>();

        LayoutManager::registerLoadCallback(
            [](std::string_view line)
            {
                for (auto& [name, view] :
                    ContentRegistry::Views::impl::getEntries())
                {
                    if (!view->shouldStoreWindowState())
                        continue;

                    std::string format =
                        fmt::format("{}=%d", view->getUnlocalizedName().get());
                    sscanf(line.data(),
                        format.c_str(),
                        &view->getWindowOpenState());
                }
            });

        LayoutManager::registerStoreCallback(
            [](ImGuiTextBuffer* buffer)
            {
                for (auto& [name, view] :
                    ContentRegistry::Views::impl::getEntries())
                {
                    if (!view->shouldStoreWindowState())
                        continue;

                    buffer->appendf("%s=%d\n",
                        name.get().c_str(),
                        view->getWindowOpenState());
                }
            });
    }
}

#define IMHEX_VERSION ""
#define IMHEX_PLUGIN_NAME fluxengine
IMHEX_PLUGIN_SETUP_BUILTIN("Built-in", "david.given", "FluxEngine TC")
{
    using namespace hex::plugin::builtin;

    ImHexApi::System::addStartupTask(
        "Load Window Settings", false, loadWindowSettings);
    ImHexApi::System::addStartupTask(
        "Configuring UI scale", false, configureUIScale);

    extractBundledFiles();

    hex::log::debug("Using romfs: '{}'", romfs::name());
    LocalizationManager::addLanguages(
        romfs::get("lang/languages.json").string(),
        [](const std::filesystem::path& path)
        {
            return romfs::get(path).string();
        });
    LocalizationManager::setLanguage("native");

    addFooterItems();
    addTitleBarButtons();
    addToolbarItems();
    addGlobalUIItems();

    registerEventHandlers();
    registerDataVisualizers();
    registerThemes();
    registerSettings();
    loadSettings();
    registerThemeHandlers();
    registerStyleHandlers();
    ContentRegistry::Provider::add<NullProvider>(true);
    registerViews();
    registerMainMenuEntries();
    registerThemeHandlers();
    registerStyleHandlers();
    registerProjectHandlers();
    loadWorkspaces();
    addWindowDecoration();

    auto provider =
        ImHexApi::Provider::createProvider("hex.builtin.provider.null");
    if (provider != nullptr)
        std::ignore = provider->open();
        
        ContentRegistry::Settings::onChange("hex.builtin.setting.interface", "hex.builtin.setting.interface.color", [](const ContentRegistry::Settings::SettingsValue &value) {
            auto theme = value.get<std::string>("Dark");
            if (theme != ThemeManager::NativeTheme) {
                static std::string lastTheme;

                if (theme != lastTheme) {
                    RequestChangeTheme::post(theme);
                    lastTheme = theme;
                }
            }
        });
        ContentRegistry::Settings::onChange("hex.builtin.setting.interface", "hex.builtin.setting.interface.language", [](const ContentRegistry::Settings::SettingsValue &value) {
            auto language = value.get<std::string>("en-US");
            if (language != LocalizationManager::getSelectedLanguageId())
                LocalizationManager::setLanguage(language);
        });
        ContentRegistry::Settings::onChange("hex.builtin.setting.interface", "hex.builtin.setting.interface.fps", [](const ContentRegistry::Settings::SettingsValue &value) {
            ImHexApi::System::setTargetFPS(static_cast<float>(value.get<int>(14)));
        });

            LayoutManager::loadFromString(std::string(romfs::get("layouts/default.hexlyt").string()));
}

extern "C" void forceLinkPlugin_Fonts();

namespace
{
    struct StaticLoad
    {
        StaticLoad()
        {
            forceLinkPlugin_fluxengine();
            forceLinkPlugin_Fonts();
        }
    };
}

static StaticLoad staticLoad;
