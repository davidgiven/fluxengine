#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include <imgui_internal.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "controlpanelview.h"
#include "datastore.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

static DynamicSettingFactory settings("fluxengine.settings");

ControlPanelView::ControlPanelView():
    View::Window("fluxengine.view.controlpanel.name", ICON_VS_COMPASS)
{
}

static void loadFluxFile()
{
    fs::openFileBrowser(fs::DialogMode::Open,
        {},
        [](const auto& path)
        {
            settings.get<std::string>("device") = DEVICE_FLUXFILE;
            settings.get<std::fs::path>("fluxfile") = path;

            Datastore::beginRead();
        });
}

static void saveSectorImage()
{
    fs::openFileBrowser(fs::DialogMode::Save, {}, Datastore::writeImage);
}

static void saveFluxFile()
{
    fs::openFileBrowser(fs::DialogMode::Save, {}, Datastore::writeFluxFile);
}

void ControlPanelView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();
    bool busy = Datastore::isBusy();
    bool hasImage = diskFlux && diskFlux->image;

    if (ImGui::BeginTable("controlPanelOuter",
            3,
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoClip))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndTable();
        };

        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableNextColumn();
        ImGui::TextAligned(0.5f, -FLT_MIN, "Inputs");
        ImGui::TableNextColumn();
        ImGui::TableHeader(ICON_VS_ARROW_RIGHT);
        ImGui::TableNextColumn();
        ImGui::TextAligned(0.5f, -FLT_MIN, "Outputs");

        auto button = [&](const std::string& icon,
                          const std::string& label,
                          std::function<void()> callback,
                          bool isDisabled)
        {
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(isDisabled);
            ON_SCOPE_EXIT
            {
                ImGui::EndDisabled();
            };
            if (ImGui::Button(fmt::format("{} {}", icon, label).c_str(),
                    {ImGui::GetContentRegionAvail().x, 0}))
                callback();
        };

        ImGui::TableNextRow();
        button(ICON_TA_DEVICE_FLOPPY,
            "fluxengine.views.controlpanel.readDevice"_lang,
            Datastore::beginRead,
            busy);
        ImGui::TableNextColumn();
        button(ICON_VS_SAVE_AS,
            "fluxengine.views.controlpanel.writeDevice"_lang,
            nullptr,
            busy || !hasImage);

        ImGui::TableNextRow();
        button(ICON_TA_UPLOAD,
            "fluxengine.views.controlpanel.readFlux"_lang,
            loadFluxFile,
            busy);
        ImGui::TableNextColumn();
        button(ICON_TA_DOWNLOAD,
            "fluxengine.views.controlpanel.writeFlux"_lang,
            saveFluxFile,
            busy || !diskFlux);

        ImGui::TableNextRow();
        button(ICON_VS_FOLDER_OPENED,
            "fluxengine.views.controlpanel.readImage"_lang,
            nullptr,
            busy);
        ImGui::TableNextColumn();
        button(ICON_VS_SAVE_ALL,
            "fluxengine.views.controlpanel.writeImage"_lang,
            saveSectorImage,
            busy || !hasImage);

        ImGui::TableNextRow();
        button(ICON_TA_REPEAT,
            "fluxengine.views.controlpanel.rereadBad"_lang,
            nullptr,
            busy || !diskFlux);

        ImGui::TableNextRow();
        button(ICON_VS_NEW_FILE,
            "fluxengine.views.controlpanel.createBlank"_lang,
            nullptr,
            busy || !Datastore::canFormat());
    }

    ImGui::Dummy({0, ImGui::GetFontSize()});

    {
        auto red = ImGuiExt::GetCustomColorU32(ImGuiCustomCol_LoggerError);
        auto text = ImGui::GetColorU32(ImGuiCol_Text);
        auto redHover = ImMixU32(red, text, 32);
        auto redPressed = ImMixU32(red, text, 64);

        ImGui::PushStyleColor(ImGuiCol_Button, red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, redPressed);

        ON_SCOPE_EXIT
        {
            ImGui::PopStyleColor(3);
        };

        if (maybeDisabledButton(fmt::format("{} {}",
                                    ICON_TA_CANCEL,
                                    "fluxengine.summary.controls.stop"_lang),
                {ImGui::GetContentRegionAvail().x, 0},
                !busy))
            Datastore::stop();
    }
}
