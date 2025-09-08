#include <hex.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <hex/ui/imgui_imhex_extensions.h>
#include <hex/api/project_file_manager.hpp>
#include <hex/api/events/events_provider.hpp>
#include <hex/api/events/events_gui.hpp>
#include <hex/api/events/requests_gui.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/content_registry/views.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/api/layout_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/helpers/default_paths.hpp>
#include <toasts/toast_notification.hpp>
#include <content/recent.hpp>
#include <wolv/io/file.hpp>
#include <wolv/io/fs.hpp>
#include <nlohmann/json.hpp>
#include <romfs/romfs.hpp>

namespace hex::plugin::builtin
{
    using namespace hex;

    void setupOutOfBoxExperience() {}

    void loadDefaultLayout()
    {
        LayoutManager::loadFromString(
            std::string(romfs::get("layouts/default.hexlyt").string()));
    }

    bool isAnyViewOpen()
    {
        const auto& views = ContentRegistry::Views::impl::getEntries();
        return std::any_of(views.begin(),
            views.end(),
            [](const auto& entry)
            {
                return entry.second->getWindowOpenState();
            });
    }

    void drawNoViewsBackground()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_WindowShadow, 0x00);
        if (ImGui::Begin("ImHexDockSpace",
                nullptr,
                ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            static std::array<char, 256> title;
            ImFormatString(title.data(),
                title.size(),
                "%s/DockSpace_%08X",
                ImGui::GetCurrentWindowRead()->Name,
                ImGui::GetID("ImHexMainDock"));
            if (ImGui::Begin(title.data()))
            {
                ImGui::Dummy({});
                ImGui::PushStyleVar(
                    ImGuiStyleVar_WindowPadding, scaled({10, 10}));

                ImGui::SetNextWindowScroll({0.0F, -1.0F});
                ImGui::SetNextWindowSize(
                    ImGui::GetContentRegionAvail() + scaled({0, 10}));
                ImGui::SetNextWindowPos(
                    ImGui::GetCursorScreenPos() -
                    ImVec2(0, ImGui::GetStyle().FramePadding.y + 2_scaled));
                ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
                if (ImGui::Begin("Welcome Screen",
                        nullptr,
                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                            ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoDocking |
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove))
                {
                    auto imageSize = scaled(ImVec2(350, 350));
                    auto imagePos =
                        (ImGui::GetContentRegionAvail() - imageSize) / 2;

                    auto loadDefaultText =
                        "hex.builtin.layouts.none.restore_default"_lang;
                    auto textSize = ImGui::CalcTextSize(loadDefaultText);

                    auto textPos = ImVec2(
                        (ImGui::GetContentRegionAvail().x - textSize.x) / 2,
                        imagePos.y + imageSize.y);

                    ImGui::SetCursorPos(textPos);
                    if (ImGuiExt::DimmedButton(loadDefaultText))
                    {
                        loadDefaultLayout();
                    }
                }

                ImGui::End();
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    class PopupRestoreBackup : public Popup<PopupRestoreBackup>
    {
    private:
        std::fs::path m_logFilePath;
        std::function<void()> m_restoreCallback;
        std::function<void()> m_deleteCallback;

    public:
        PopupRestoreBackup(std::fs::path logFilePath,
            const std::function<void()>& restoreCallback,
            const std::function<void()>& deleteCallback):
            Popup("hex.builtin.popup.safety_backup.title"),
            m_logFilePath(std::move(logFilePath)),
            m_restoreCallback(restoreCallback),
            m_deleteCallback(deleteCallback)
        {
        }

        void drawContent() override
        {
            ImGui::TextUnformatted("hex.builtin.popup.safety_backup.desc"_lang);
            if (!m_logFilePath.empty())
            {
                ImGui::NewLine();
                ImGui::TextUnformatted(
                    "hex.builtin.popup.safety_backup.log_file"_lang);
                ImGui::SameLine(0, 2_scaled);
                if (ImGuiExt::Hyperlink(
                        m_logFilePath.filename().string().c_str()))
                {
                    fs::openFolderWithSelectionExternal(m_logFilePath);
                }

                ImGui::NewLine();
            }

            auto width = ImGui::GetWindowWidth();
            ImGui::SetCursorPosX(width / 9);
            if (ImGui::Button("hex.builtin.popup.safety_backup.restore"_lang,
                    ImVec2(width / 3, 0)))
            {
                m_restoreCallback();
                m_deleteCallback();

                this->close();
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(width / 9 * 5);
            if (ImGui::Button("hex.builtin.popup.safety_backup.delete"_lang,
                    ImVec2(width / 3, 0)) ||
                ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                m_deleteCallback();

                this->close();
            }
        }
    };

    void createWelcomeScreen()
    {
        recent::registerEventHandlers();
        recent::updateRecentEntries();

        // Sets a background when they are no views
        EventFrameBegin::subscribe(
            []
            {
                if (ImHexApi::Provider::isValid() && !isAnyViewOpen())
                    drawNoViewsBackground();
            });

        ContentRegistry::Settings::onChange("hex.builtin.setting.interface",
            "hex.builtin.setting.interface.color",
            [](const ContentRegistry::Settings::SettingsValue& value)
            {
                auto theme = value.get<std::string>("Dark");
                if (theme != ThemeManager::NativeTheme)
                {
                    static std::string lastTheme;

                    if (theme != lastTheme)
                    {
                        RequestChangeTheme::post(theme);
                        lastTheme = theme;
                    }
                }
            });
        ContentRegistry::Settings::onChange("hex.builtin.setting.interface",
            "hex.builtin.setting.interface.language",
            [](const ContentRegistry::Settings::SettingsValue& value)
            {
                auto language = value.get<std::string>("en-US");
                if (language != LocalizationManager::getSelectedLanguageId())
                    LocalizationManager::setLanguage(language);
            });
        ContentRegistry::Settings::onChange("hex.builtin.setting.interface",
            "hex.builtin.setting.interface.fps",
            [](const ContentRegistry::Settings::SettingsValue& value)
            {
                ImHexApi::System::setTargetFPS(
                    static_cast<float>(value.get<int>(14)));
            });

        // Check for crash backup
        constexpr static auto CrashFileName = "crash.json";
        constexpr static auto BackupFileName = "crash_backup.hexproj";
        bool hasCrashed = false;

        for (const auto& path : paths::Config.read())
        {
            if (auto crashFilePath = std::fs::path(path) / CrashFileName;
                wolv::io::fs::exists(crashFilePath))
            {
                hasCrashed = true;

                log::info("Found crash.json file at {}",
                    wolv::util::toUTF8String(crashFilePath));
                wolv::io::File crashFile(
                    crashFilePath, wolv::io::File::Mode::Read);
                nlohmann::json crashFileData;
                try
                {
                    crashFileData =
                        nlohmann::json::parse(crashFile.readString());
                }
                catch (nlohmann::json::exception& e)
                {
                    log::error("Failed to parse crash.json file: {}", e.what());
                    crashFile.remove();
                    continue;
                }

                bool hasProject = !crashFileData.value("project", "").empty();

                auto backupFilePath = path / BackupFileName;
                auto backupFilePathOld = path / BackupFileName;
                backupFilePathOld.replace_extension(".hexproj.old");

                bool hasBackupFile = wolv::io::fs::exists(backupFilePath);

                if (!hasProject && !hasBackupFile)
                {
                    log::warn(
                        "No project file or backup file found in crash.json "
                        "file");

                    crashFile.close();

                    // Delete crash.json file
                    wolv::io::fs::remove(crashFilePath);

                    // Delete old backup file
                    wolv::io::fs::remove(backupFilePathOld);

                    // Try to move current backup file to the old backup
                    // location
                    if (wolv::io::fs::copyFile(
                            backupFilePath, backupFilePathOld))
                    {
                        wolv::io::fs::remove(backupFilePath);
                    }
                    continue;
                }

                PopupRestoreBackup::open(
                    // Path of log file
                    crashFileData.value("logFile", ""),

                    // Restore callback
                    [crashFileData, backupFilePath, hasProject, hasBackupFile]
                    {
                        if (hasBackupFile)
                        {
                            if (ProjectFile::load(backupFilePath))
                            {
                                if (hasProject)
                                {
                                    ProjectFile::setPath(
                                        crashFileData["project"]
                                            .get<std::string>());
                                }
                                else
                                {
                                    ProjectFile::setPath("");
                                }
                                RequestUpdateWindowTitle::post();
                            }
                            else
                            {
                                ui::ToastError::open(fmt::format(
                                    "hex.builtin.popup.error.project.load"_lang,
                                    wolv::util::toUTF8String(backupFilePath)));
                            }
                        }
                        else
                        {
                            if (hasProject)
                            {
                                ProjectFile::setPath(crashFileData["project"]
                                        .get<std::string>());
                            }
                        }
                    },

                    // Delete callback (also executed after restore)
                    [crashFilePath, backupFilePath]
                    {
                        wolv::io::fs::remove(crashFilePath);
                        wolv::io::fs::remove(backupFilePath);
                    });
            }
        }
    }

}
