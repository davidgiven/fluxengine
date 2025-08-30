#include <hex/api/imhex_api/provider.hpp>
#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/content_registry/data_formatter.hpp>
#include <hex/api/content_registry/reports.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/content_registry/settings.hpp>

#include <imgui.h>
#include <implot.h>

#include <hex/ui/view.hpp>
#include <hex/api/shortcut_manager.hpp>
#include <hex/api/project_file_manager.hpp>
#include <hex/api/layout_manager.hpp>
#include <hex/api/achievement_manager.hpp>
#include <hex/api/events/requests_gui.hpp>
#include <hex/api/workspace_manager.hpp>
#include <hex/api/events/events_interaction.hpp>

#include <hex/helpers/crypto.hpp>
#include <hex/helpers/patches.hpp>
#include <hex/providers/provider.hpp>

#include <content/global_actions.hpp>
#include <toasts/toast_notification.hpp>
#include <popups/popup_text_input.hpp>

#include <wolv/io/file.hpp>
#include <wolv/literals.hpp>

#include <romfs/romfs.hpp>
#include <hex/helpers/menu_items.hpp>

#include <GLFW/glfw3.h>
#include <popups/popup_question.hpp>

using namespace std::literals::string_literals;
using namespace wolv::literals;

namespace hex::plugin::builtin
{

    namespace
    {

        bool noRunningTasks()
        {
            return TaskManager::getRunningTaskCount() == 0;
        }

        bool noRunningTaskAndValidProvider()
        {
            return noRunningTasks() && ImHexApi::Provider::isValid();
        }

        bool noRunningTaskAndWritableProvider()
        {
            return noRunningTasks() && ImHexApi::Provider::isValid() &&
                   ImHexApi::Provider::get()->isWritable();
        }

    }

    static void createFileMenu()
    {

        ContentRegistry::UserInterface::registerMainMenuItem(
            "hex.builtin.menu.file", 1000);

        /* Project open / save */
        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.file", "hex.builtin.menu.file.project"},
            ICON_VS_NOTEBOOK,
            1400,
            []
            {
            },
            noRunningTasks);

        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.file",
                "hex.builtin.menu.file.project",
                "hex.builtin.menu.file.project.open"},
            ICON_VS_ROOT_FOLDER_OPENED,
            1410,
            CTRL + ALT + Keys::O + AllowWhileTyping,
            openProject,
            noRunningTasks);

        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.file",
                "hex.builtin.menu.file.project",
                "hex.builtin.menu.file.project.save"},
            ICON_VS_SAVE,
            1450,
            CTRL + ALT + Keys::S + AllowWhileTyping,
            saveProject,
            [&]
            {
                return noRunningTaskAndValidProvider() &&
                       ProjectFile::hasPath();
            });

        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.file",
                "hex.builtin.menu.file.project",
                "hex.builtin.menu.file.project.save_as"},
            ICON_VS_SAVE_AS,
            1500,
            ALT + SHIFT + Keys::S + AllowWhileTyping,
            saveProjectAs,
            noRunningTaskAndValidProvider);

        ContentRegistry::UserInterface::addMenuItemSeparator(
            {"hex.builtin.menu.file"}, 2000);

        /* Quit ImHex */
        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.file", "hex.builtin.menu.file.quit"},
            ICON_VS_CLOSE_ALL,
            10100,
            ALT + Keys::F4 + AllowWhileTyping,
            []
            {
                ImHexApi::System::closeImHex();
            });
    }

    static void createEditMenu()
    {
        ContentRegistry::UserInterface::registerMainMenuItem(
            "hex.builtin.menu.edit", 2000);
    }

    static void createViewMenu()
    {
        ContentRegistry::UserInterface::registerMainMenuItem(
            "hex.builtin.menu.view", 3000);

#if !defined(OS_WEB)
        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.view", "hex.builtin.menu.view.always_on_top"},
            ICON_VS_PINNED,
            1000,
            Keys::F10 + AllowWhileTyping,
            []
            {
                static bool state = false;

                state = !state;
                glfwSetWindowAttrib(ImHexApi::System::getMainWindowHandle(),
                    GLFW_FLOATING,
                    state);
            },
            []
            {
                return true;
            },
            []
            {
                return glfwGetWindowAttrib(
                    ImHexApi::System::getMainWindowHandle(), GLFW_FLOATING);
            });
#endif

#if !defined(OS_MACOS) && !defined(OS_WEB)
        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.view", "hex.builtin.menu.view.fullscreen"},
            ICON_VS_SCREEN_FULL,
            2000,
            Keys::F11 + AllowWhileTyping,
            []
            {
                static bool state = false;
                static ImVec2 position, size;

                state = !state;

                const auto window = ImHexApi::System::getMainWindowHandle();
                if (state)
                {
                    position = ImHexApi::System::getMainWindowPosition();
                    size = ImHexApi::System::getMainWindowSize();

                    const auto monitor = glfwGetPrimaryMonitor();
                    const auto videoMode = glfwGetVideoMode(monitor);

                    glfwSetWindowMonitor(window,
                        monitor,
                        0,
                        0,
                        videoMode->width,
                        videoMode->height,
                        videoMode->refreshRate);
                }
                else
                {
                    glfwSetWindowMonitor(window,
                        nullptr,
                        position.x,
                        position.y,
                        size.x,
                        size.y,
                        0);
                    glfwSetWindowAttrib(window,
                        GLFW_DECORATED,
                        ImHexApi::System::isBorderlessWindowModeEnabled()
                            ? GLFW_FALSE
                            : GLFW_TRUE);
                }
            },
            []
            {
                return true;
            },
            []
            {
                return glfwGetWindowMonitor(
                           ImHexApi::System::getMainWindowHandle()) != nullptr;
            });
#endif

#if !defined(OS_WEB)
        ContentRegistry::UserInterface::addMenuItemSeparator(
            {"hex.builtin.menu.view"}, 3000);
#endif

        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.view"},
            4000,
            []
            {
                for (auto& [name, view] :
                    ContentRegistry::Views::impl::getEntries())
                {
                    if (view->hasViewMenuItemEntry())
                    {
                        auto& state = view->getWindowOpenState();

                        if (menu::menuItemEx(Lang(view->getUnlocalizedName()),
                                view->getIcon(),
                                Shortcut::None,
                                state,
                                ImHexApi::Provider::isValid() &&
                                    !LayoutManager::isLayoutLocked()))
                            state = !state;
                    }
                }
            });
    }

    static void createLayoutMenu()
    {
        LayoutManager::reload();

        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.workspace", "hex.builtin.menu.workspace.layout"},
            ICON_VS_LAYOUT,
            1050,
            []
            {
            },
            ImHexApi::Provider::isValid);

        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.workspace",
                "hex.builtin.menu.workspace.layout",
                "hex.builtin.menu.workspace.layout.save"},
            1100,
            Shortcut::None,
            []
            {
                ui::PopupTextInput::open("hex.builtin.popup.save_layout.title",
                    "hex.builtin.popup.save_layout.desc",
                    [](const std::string& name)
                    {
                        LayoutManager::save(name);
                    });
            },
            ImHexApi::Provider::isValid);

        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.workspace", "hex.builtin.menu.workspace.layout"},
            ICON_VS_LAYOUT,
            1150,
            []
            {
                bool locked = LayoutManager::isLayoutLocked();
                if (menu::menuItemEx(
                        "hex.builtin.menu.workspace.layout.lock"_lang,
                        ICON_VS_LOCK,
                        Shortcut::None,
                        locked,
                        ImHexApi::Provider::isValid()))
                {
                    LayoutManager::lockLayout(!locked);
                    ContentRegistry::Settings::write<bool>(
                        "hex.builtin.setting.interface",
                        "hex.builtin.setting.interface.layout_locked",
                        !locked);
                }
            });

        ContentRegistry::UserInterface::addMenuItemSeparator(
            {"hex.builtin.menu.workspace", "hex.builtin.menu.workspace.layout"},
            1200);

        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.workspace", "hex.builtin.menu.workspace.layout"},
            2000,
            []
            {
                for (const auto& path : romfs::list("layouts"))
                {
                    if (menu::menuItem(
                            wolv::util::capitalizeString(path.stem().string())
                                .c_str(),
                            Shortcut::None,
                            false,
                            ImHexApi::Provider::isValid()))
                    {
                        LayoutManager::loadFromString(
                            std::string(romfs::get(path).string()));
                    }
                }

                bool shiftPressed = ImGui::GetIO().KeyShift;
                for (auto& [name, path] : LayoutManager::getLayouts())
                {
                    if (menu::menuItem(
                            fmt::format("{}{}",
                                name,
                                shiftPressed ? " " ICON_VS_CHROME_CLOSE : "")
                                .c_str(),
                            Shortcut::None,
                            false,
                            ImHexApi::Provider::isValid()))
                    {
                        if (shiftPressed)
                        {
                            LayoutManager::removeLayout(name);
                            break;
                        }
                        else
                        {
                            LayoutManager::load(path);
                        }
                    }
                }
            });
    }

    static void createWorkspaceMenu()
    {
        ContentRegistry::UserInterface::registerMainMenuItem(
            "hex.builtin.menu.workspace", 4000);

        createLayoutMenu();

        ContentRegistry::UserInterface::addMenuItemSeparator(
            {"hex.builtin.menu.workspace"}, 3000);

        ContentRegistry::UserInterface::addMenuItem(
            {"hex.builtin.menu.workspace", "hex.builtin.menu.workspace.create"},
            ICON_VS_ADD,
            3100,
            Shortcut::None,
            []
            {
                ui::PopupTextInput::open(
                    "hex.builtin.popup.create_workspace.title",
                    "hex.builtin.popup.create_workspace.desc",
                    [](const std::string& name)
                    {
                        WorkspaceManager::createWorkspace(name);
                    });
            },
            ImHexApi::Provider::isValid);

        ContentRegistry::UserInterface::addMenuItemSubMenu(
            {"hex.builtin.menu.workspace"},
            3200,
            []
            {
                const auto& workspaces = WorkspaceManager::getWorkspaces();

                bool shiftPressed = ImGui::GetIO().KeyShift;
                for (auto it = workspaces.begin(); it != workspaces.end(); ++it)
                {
                    const auto& [name, workspace] = *it;

                    bool canRemove = shiftPressed && !workspace.builtin;
                    if (menu::menuItem(
                            fmt::format("{}{}",
                                name,
                                canRemove ? " " ICON_VS_CHROME_CLOSE : "")
                                .c_str(),
                            Shortcut::None,
                            it == WorkspaceManager::getCurrentWorkspace(),
                            ImHexApi::Provider::isValid()))
                    {
                        if (canRemove)
                        {
                            WorkspaceManager::removeWorkspace(name);
                            break;
                        }
                        else
                        {
                            WorkspaceManager::switchWorkspace(name);
                        }
                    }
                }
            });
    }

    static void createExtrasMenu() {
        ContentRegistry::UserInterface::registerMainMenuItem("hex.builtin.menu.extras", 5000);

        ContentRegistry::UserInterface::addMenuItemSeparator({ "hex.builtin.menu.extras" }, 2600);
    }

    static void createHelpMenu()
    {
        ContentRegistry::UserInterface::registerMainMenuItem(
            "hex.builtin.menu.help", 6000);
    }

    void registerMainMenuEntries()
    {
        createFileMenu();
        createEditMenu();
        createViewMenu();
        createWorkspaceMenu();
        createExtrasMenu();
        createHelpMenu();
    }
}