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
            hex::ContentRegistry::Settings::write<std::fs::path>(
                FLUXENGINE_CONFIG,
                "fluxengine.settings.device",
                DEVICE_FLUXFILE);
            hex::ContentRegistry::Settings::write<std::fs::path>(
                FLUXENGINE_CONFIG, "fluxengine.settings.fluxfile", path);
        });
}

static void saveSectorImage()
{
    fs::openFileBrowser(fs::DialogMode::Save, {}, Datastore::writeImage);
}

static void showOptions() {}

static void emitOptions(const OptionsMap& options,
    const ConfigProto* config,
    const std::set<int>& applicableOptions)
{
    for (auto& it : config->option())
    {
        if (!applicableOptions.empty() &&
            std::ranges::none_of(it.applicability(),
                [&](int item)
                {
                    return applicableOptions.contains(item);
                }))
            continue;

        ImGui::Text(fmt::format("{}: {}",
            wolv::util::capitalizeString(it.comment()),
            options.contains(it.name()) ? "fluxengine.view.summary.yes"_lang
                                        : "fluxengine.view.summary.no"_lang));
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

        ImGui::Text(fmt::format("{}:", comment));
        ImGui::SameLine();
        if (selectedOption)
            ImGui::TextWrapped(
                wolv::util::capitalizeString(selectedOption->comment())
                    .c_str());
        else
            ImGui::TextWrapped("***bad***");
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
        auto deviceName = hex::ContentRegistry::Settings::read<std::string>(
            FLUXENGINE_CONFIG, "fluxengine.settings.device", DEVICE_FLUXFILE);
        auto device = findOrDefault(Datastore::getDevices(),
            deviceName,
            {.label = "No device configured"});
        if (deviceName == DEVICE_FLUXFILE)
        {
            ImGui::Text(fmt::format("{}: {}",
                device.label,
                hex::ContentRegistry::Settings::read<std::fs::path>(
                    FLUXENGINE_CONFIG,
                    "fluxengine.settings.fluxfile",
                    "no file configured")
                    .filename()
                    .string()));
            applicableOptions.insert(FLUXFILE_SOURCESINK);
        }
        else if (deviceName == DEVICE_MANUAL)
        {
            ImGui::Text(fmt::format("Greaseweazle: {}",
                hex::ContentRegistry::Settings::read<std::string>(
                    FLUXENGINE_CONFIG,
                    "fluxengine.settings.manualDevicePath",
                    "no path configured")));
            applicableOptions.insert(MANUAL_SOURCESINK);
            applicableOptions.insert(HARDWARE_SOURCESINK);
        }
        else
        {
            ImGui::Text(device.label.c_str());
            applicableOptions.insert(HARDWARE_SOURCESINK);
        }

        /* Other options */

        auto globalOptions =
            stringToOptions(hex::ContentRegistry::Settings::read<std::string>(
                FLUXENGINE_CONFIG, "fluxengine.settings.globalSettings", ""));
        emitOptions(
            globalOptions, formats.at("_global_options"), applicableOptions);

        ImGui::Button(
            fmt::format("{}##device", "fluxengine.view.summary.edit"_lang)
                .c_str());
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

        auto formatName =
            hex::ContentRegistry::Settings::read<std::string>(FLUXENGINE_CONFIG,
                "fluxengine.settings.format.selected",
                DEVICE_FLUXFILE);
        auto format = findOrDefault(formats, formatName);
        if (!format)
        {
            ImGui::Text("***bad***");
            return;
        }

        ImGui::Text(fmt::format("{}:", "fluxengine.view.summary.format"_lang));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text(format->shortname());
        ImGui::Text(format->comment());
        ImGui::EndGroup();

        auto formatOptions = stringToOptions(
            hex::ContentRegistry::Settings::read<std::string>(FLUXENGINE_CONFIG,
                fmt::format("fluxengine.settings.{}", formatName),
                ""));
        emitOptions(formatOptions, format, {});

        ImGui::Button(
            fmt::format("{}##format", "fluxengine.view.summary.edit"_lang)
                .c_str());
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
        ImGui::GetContentRegionAvail().y - ImGui::GetFontSize() * 6);
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