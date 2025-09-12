#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "summaryview.h"
#include "datastore.h"
#include "configview.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

static DynamicSettingFactory settings("fluxengine.settings");

SummaryView::SummaryView():
    View::Window("fluxengine.view.summary.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
}

static std::optional<std::set<std::shared_ptr<const Sector>>> findSectors(
    std::shared_ptr<const DiskFlux>& diskFlux, int cylinder, int head)
{
    if (diskFlux)
        for (auto& it : diskFlux->tracks)
        {
            if ((it->trackInfo->physicalCylinder == cylinder) &&
                (it->trackInfo->physicalHead == head))
                return std::make_optional(it->sectors);
        }

    return {};
}

static void loadFluxFile()
{
    fs::openFileBrowser(fs::DialogMode::Open,
        {},
        [](const auto& path)
        {
            settings.get<std::string>("device") = DEVICE_FLUXFILE;
            settings.get<std::fs::path>("fluxfile") = path;
        });
}

static void saveSectorImage()
{
    fs::openFileBrowser(fs::DialogMode::Save, {}, Datastore::writeImage);
}

static void showOptions() {}

static void emitOptions(DynamicSetting<std::string>& setting,
    const ConfigProto* config,
    const std::set<int>& applicableOptions)
{
    auto options = stringToOptions(setting);

    for (auto& it : config->option())
    {
        if (!applicableOptions.empty() &&
            std::ranges::none_of(it.applicability(),
                [&](int item)
                {
                    return applicableOptions.contains(item);
                }))
            continue;

        ImGui::AlignTextToFramePadding();
        bool selected = options.contains(it.name());
        ImGui::Text(fmt::format("{}: {}",
            wolv::util::capitalizeString(it.comment()),
            selected ? "fluxengine.view.summary.yes"_lang
                     : "fluxengine.view.summary.no"_lang));
        ImGui::SameLine();
        if (ImGui::BeginCombo(fmt::format("##{}", it.comment()).c_str(),
                nullptr,
                ImGuiComboFlags_NoPreview))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            int value = -1;
            if (ImGui::Selectable("fluxengine.view.summary.no"_lang, !selected))
                value = 0;
            if (ImGui::Selectable("fluxengine.view.summary.yes"_lang, selected))
                value = 1;
            if (value != -1)
            {
                options.erase(it.name());
                if (value)
                    options[it.name()] = "true";
                setting = optionsToString(options);
            }
        }
    }

    for (auto& it : config->option_group())
    {
        if (!applicableOptions.empty() &&
            std::ranges::none_of(it.applicability(),
                [&](int item)
                {
                    return applicableOptions.contains(item);
                }))
            continue;

        std::string comment = it.comment();
        if (comment == "$formats")
            comment = (std::string) "fluxengine.view.summary.variations"_lang;

        const OptionProto* selectedOption = nullptr;
        if (it.name().empty())
        {
            for (auto& ot : it.option())
                if (options.contains(ot.name()))
                    selectedOption = &ot;
        }
        else
        {
            auto value = findOrDefault(options, it.name());
            for (auto& ot : it.option())
                if (ot.name() == value)
                    selectedOption = &ot;
        }

        if (!selectedOption)
            for (auto& ot : it.option())
                if (ot.set_by_default())
                    selectedOption = &ot;

        ImGui::AlignTextToFramePadding();
        ImGui::Text(fmt::format("{}:", comment));
        ImGui::SameLine();
        if (selectedOption)
            ImGui::TextWrapped(
                wolv::util::capitalizeString(selectedOption->comment())
                    .c_str());
        else
            ImGui::TextWrapped("***bad***");
        ImGui::SameLine();
        if (ImGui::BeginCombo(fmt::format("##{}", it.comment()).c_str(),
                nullptr,
                ImGuiComboFlags_NoPreview))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& ot : it.option())
                if (ImGui::Selectable(
                        ot.comment().c_str(), &ot == selectedOption))
                {
                    if (it.name().empty())
                    {
                        options.erase(selectedOption->name());
                        options[ot.name()] = "true";
                    }
                    else
                    {
                        options.erase(it.name());
                        options[it.name()] = ot.name();
                    }
                    setting = optionsToString(options);
                }
        }
    }
}

static void drawDeviceBox()
{
    if (ImGuiExt::BeginSubWindow("Device",
            nullptr,
            ImGui::GetContentRegionAvail(),
            ImGuiChildFlags_None))
    {
        ON_SCOPE_EXIT
        {
            ImGuiExt::EndSubWindow();
        };

        /* Device name */

        std::set<int> applicableOptions = {ANY_SOURCESINK};
        auto deviceNameSetting = settings.get<std::string>("device");
        auto selectedDevice = findOrDefault(Datastore::getDevices(),
            (std::string)deviceNameSetting,
            {.label = "No device configured"});
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Device:");
        ImGui::SameLine();
        constexpr int maxLen = 25;
        if ((std::string)deviceNameSetting == DEVICE_FLUXFILE)
        {
            ImGui::Text(shortenString(selectedDevice.label, maxLen));
            applicableOptions.insert(FLUXFILE_SOURCESINK);
        }
        else if ((std::string)deviceNameSetting == DEVICE_MANUAL)
        {
            ImGui::Text("Greaseweazle manual setup");

            applicableOptions.insert(MANUAL_SOURCESINK);
            applicableOptions.insert(HARDWARE_SOURCESINK);
        }
        else
        {
            ImGui::Text(shortenString(selectedDevice.label, maxLen));
            applicableOptions.insert(HARDWARE_SOURCESINK);
        }
        ImGui::SameLine();
        if (ImGui::BeginCombo("##devices", nullptr, ImGuiComboFlags_NoPreview))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& [name, device] : Datastore::getDevices())
                if (ImGui::Selectable(device.label.c_str(), false))
                    deviceNameSetting = name;
        }

        /* The file path, if DEVICE_FLUXFILE, and device path, if DEVICE_MANUAL
         */

        auto doPathSetting = [=](const std::string& label,
                                 const std::string& setting,
                                 const std::string& id)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text(fmt::format("{}:", label));
            ImGui::SameLine();
            auto pathSetting = settings.get<std::string>(setting);
            auto pathString = (std::string)pathSetting;
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText(id.c_str(),
                    pathString,
                    ImGuiInputTextFlags_CallbackCompletion |
                        ImGuiInputTextFlags_ElideLeft))
                pathSetting = pathString;
        };

        if ((std::string)deviceNameSetting == DEVICE_FLUXFILE)
            doPathSetting("fluxengine.view.summary.fluxFile"_lang,
                "fluxfile",
                "##fluxfilePath");
        if ((std::string)deviceNameSetting == DEVICE_MANUAL)
            doPathSetting("fluxengine.view.summary.manualDevicePath"_lang,
                "manualDevicePath",
                "##manualDevicePath");

        /* Other options */

        auto globalOptionsSetting = DynamicSetting<std::string>(
            "fluxengine.settings", "globalSettings");
        emitOptions(globalOptionsSetting,
            formats.at("_global_options"),
            applicableOptions);
    }
}

void drawFormatBox()
{
    if (ImGuiExt::BeginSubWindow("Format",
            nullptr,
            ImGui::GetContentRegionAvail(),
            ImGuiChildFlags_None))
    {
        ON_SCOPE_EXIT
        {
            ImGuiExt::EndSubWindow();
        };

        /* Format name */

        auto formatSetting =
            settings.get<std::string>("format.selected", "ibm");
        auto selectedFormat =
            findOrDefault(formats, (std::string)formatSetting);
        if (!selectedFormat)
        {
            formatSetting = "ibm";
            selectedFormat = formats.at(formatSetting);
        }

        ImGui::Text(fmt::format("{}:", "fluxengine.view.summary.format"_lang));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextWrapped(selectedFormat->shortname().c_str());
        ImGui::TextWrapped(selectedFormat->comment().c_str());
        ImGui::EndGroup();

        ImGui::SameLine();
        if (ImGui::BeginCombo("##formats", nullptr, ImGuiComboFlags_NoPreview))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& [name, format] : formats)
                if (!format->is_extension())
                {
                    auto label = format->shortname();
                    if (label.empty())
                        label = name;
                    if (ImGui::Selectable(
                            fmt::format("{}##{}", label, name).c_str(), false))
                        formatSetting = name;
                }
        }

        auto formatOptionsSetting = DynamicSetting<std::string>(
            "fluxengine.settings", (std::string)formatSetting);
        emitOptions(formatOptionsSetting, selectedFormat, {});
    }
}

void SummaryView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();
    auto [minCylinder, maxCylinder, minHead, maxHead] =
        Datastore::getDiskPhysicalBounds();
    int numCylinders = maxCylinder - minCylinder + 1;
    int numHeads = maxHead - minHead + 1;

    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1, 1});
        ON_SCOPE_EXIT
        {
            ImGui::PopStyleVar();
        };

        auto backgroundColour = ImGui::GetColorU32(ImGuiCol_WindowBg);
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, backgroundColour);
        ON_SCOPE_EXIT
        {
            ImGui::PopStyleColor();
        };

        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, backgroundColour);
        ON_SCOPE_EXIT
        {
            ImGui::PopStyleColor();
        };

        if (ImGui::BeginTable("diskSummary",
                numCylinders + 1,
                ImGuiTableFlags_NoSavedSettings |
                    ImGuiTableFlags_HighlightHoveredColumn |
                    ImGuiTableFlags_NoClip | ImGuiTableFlags_NoPadInnerX |
                    ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Borders))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndTable();
            };

            auto originalFontSize = ImGui::GetFontSize();
            ImGui::PushFont(NULL, originalFontSize * 0.6);
            ON_SCOPE_EXIT
            {
                ImGui::PopFont();
            };

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
            ON_SCOPE_EXIT
            {
                ImGui::PopStyleVar();
            };

            float rowHeight = originalFontSize * 0.6 * 2.0;
            ImGui::TableSetupColumn(
                "", ImGuiTableColumnFlags_WidthFixed, originalFontSize);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (int cylinder = minCylinder; cylinder <= maxCylinder;
                cylinder++)
            {
                ImGui::TableNextColumn();

                auto text = fmt::format("c{}", cylinder);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     ImGui::GetColumnWidth() / 2 -
                                     ImGui::CalcTextSize(text.c_str()).x / 2);
                ImGui::Text("%s", text.c_str());
            }

            for (unsigned head = minHead; head <= maxHead; head++)
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableNextColumn();

                auto text = fmt::format("h{}", head);
                auto textSize = ImGui::CalcTextSize(text.c_str());
                ImGui::SetCursorPos({ImGui::GetCursorPosX() +
                                         ImGui::GetColumnWidth() / 2 -
                                         textSize.x / 2,
                    ImGui::GetCursorPosY() + rowHeight / 2 - textSize.y / 2});
                ImGui::Text("%s", text.c_str());

                for (unsigned cylinder = minCylinder; cylinder <= maxCylinder;
                    cylinder++)
                {
                    auto sectors = findSectors(diskFlux, cylinder, head);
                    auto colour = ImGui::GetColorU32(ImGuiCol_TextDisabled);
                    int totalSectors = 0;
                    int goodSectors = 0;
                    if (sectors.has_value())
                    {
                        totalSectors = sectors->size();
                        goodSectors = std::ranges::count_if(*sectors,
                            [](auto& e)
                            {
                                return e->status == Sector::OK;
                            });
                        colour = ImGuiExt::GetCustomColorU32(
                            ((goodSectors == totalSectors) && totalSectors)
                                ? ImGuiCustomCol_LoggerInfo
                                : ImGuiCustomCol_LoggerError);
                    }

                    ImGui::PushStyleColor(ImGuiCol_Header, colour);
                    ON_SCOPE_EXIT
                    {
                        ImGui::PopStyleColor();
                    };

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(
                            fmt::format("##c{}h{}", cylinder, head).c_str(),
                            true,
                            ImGuiSelectableFlags_None,
                            {0, rowHeight}))
                        Events::SeekToTrackViaPhysicalLocation::post(
                            CylinderHead{cylinder, head});

                    ImGui::PushFont(NULL, originalFontSize);
                    ON_SCOPE_EXIT
                    {
                        ImGui::PopFont();
                    };
                    ImGui::SetItemTooltip(
                        totalSectors
                            ? fmt::format("c{}h{}\n{} sectors read\n{} good "
                                          "sectors\n{} bad sectors",
                                  cylinder,
                                  head,
                                  totalSectors,
                                  goodSectors,
                                  totalSectors - goodSectors)
                                  .c_str()
                            : "No data");
                }
            }
        }
    }

    ImGui::SetCursorPosY(
        ImGui::GetContentRegionAvail().y - ImGui::GetFontSize() * 8);
    if (ImGui::BeginTable("controlPanelOuter",
            3,
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoClip))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndTable();
        };

        auto sideWidth = ImGui::GetFontSize() * 20;
        ImGui::TableSetupColumn(
            "", ImGuiTableColumnFlags_WidthFixed, sideWidth);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(
            "", ImGuiTableColumnFlags_WidthFixed, sideWidth);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        drawDeviceBox();

        ImGui::TableNextColumn();
        if (ImGuiExt::BeginSubWindow("Controls",
                nullptr,
                ImGui::GetContentRegionAvail(),
                ImGuiChildFlags_None))
        {
            ON_SCOPE_EXIT
            {
                ImGuiExt::EndSubWindow();
            };
            if (ImGui::BeginTable("controlPanel",
                    5,
                    ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoClip))
            {
                ON_SCOPE_EXIT
                {
                    ImGui::EndTable();
                };

                bool busy =
                    Datastore::isBusy() || !Datastore::isConfigurationValid();
                bool hasImage = diskFlux && diskFlux->image;

                auto majorButtonWidth = ImGui::GetFontSize() * 10;
                ImGui::TableSetupColumn(
                    "", ImGuiTableColumnFlags_WidthFixed, majorButtonWidth);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn(
                    "", ImGuiTableColumnFlags_WidthFixed, majorButtonWidth);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (MaybeDisabledButton(
                        fmt::format(
                            "{} {}", ICON_TA_DEVICE_FLOPPY, "Read disk"),
                        ImVec2(ImGui::GetContentRegionAvail().x, 0),
                        busy))
                    Datastore::beginRead();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                MaybeDisabledButton(
                    fmt::format(
                        "{} {}", ICON_VS_FOLDER_OPENED, "Load sector image"),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0),
                    busy);
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                MaybeDisabledButton(
                    fmt::format("{} {}", ICON_VS_SAVE_AS, "Write disk").c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0),
                    busy || !hasImage);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                MaybeDisabledButton(
                    fmt::format("{} {}", ICON_TA_REPEAT, "Reread bad tracks")
                        .c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0),
                    busy);
                ImGui::TableNextColumn();
                ImGui::Text(ICON_VS_ARROW_RIGHT);
                ImGui::TableNextColumn();
                if (MaybeDisabledButton(
                        fmt::format(
                            "{} {}", ICON_VS_SAVE_ALL, "Save sector image")
                            .c_str(),
                        ImVec2(ImGui::GetContentRegionAvail().x, 0),
                        busy || !hasImage))
                    saveSectorImage();
                ImGui::TableNextColumn();
                ImGui::Text(ICON_VS_ARROW_RIGHT);
                ImGui::TableNextColumn();
                MaybeDisabledButton(
                    fmt::format("{} {}", ICON_TA_DOWNLOAD, "Save flux file")
                        .c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0),
                    busy || !diskFlux);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (MaybeDisabledButton(
                        fmt::format("{} {}", ICON_TA_UPLOAD, "Load flux file")
                            .c_str(),
                        ImVec2(ImGui::GetContentRegionAvail().x, 0),
                        busy))
                    loadFluxFile();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                MaybeDisabledButton(
                    fmt::format("{} {}", ICON_VS_NEW_FILE, "Create blank image")
                        .c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0),
                    busy);
            }

            {
                auto size = ImVec2(ImGui::GetWindowSize().x * 0.6, 0);
                ImGui::SetCursorPos({ImGui::GetWindowSize().x / 2 - size.x / 2,
                    ImGui::GetWindowSize().y - ImGui::GetFontSize() * 1.5f});

                auto red =
                    ImGuiExt::GetCustomColorU32(ImGuiCustomCol_LoggerError);
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

                if (MaybeDisabledButton(
                        fmt::format("{} {}",
                            ICON_TA_CANCEL,
                            "fluxengine.summary.controls.stop"_lang),
                        size,
                        !Datastore::isBusy()))
                    Datastore::stop();
            }
        }

        ImGui::TableNextColumn();
        drawFormatBox();
    }
}