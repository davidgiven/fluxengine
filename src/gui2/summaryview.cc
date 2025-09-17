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
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

static DynamicSettingFactory settings("fluxengine.settings");

SummaryView::SummaryView():
    View::Window("fluxengine.view.summary.name", ICON_VS_COMPASS)
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

struct TrackAnalysis
{
    std::string tooltip;
    uint32_t colour;
};

TrackAnalysis analyseTrack(std::shared_ptr<const DiskFlux>& diskFlux,
    unsigned physicalCylinder,
    unsigned physicalHead)
{
    TrackAnalysis result = {};
    auto sectors = findSectors(diskFlux, physicalCylinder, physicalHead);
    result.colour = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    result.tooltip = "No data";
    if (sectors.has_value())
    {
        unsigned totalSectors = sectors->size();
        unsigned goodSectors = std::ranges::count_if(*sectors,
            [](auto& e)
            {
                return e->status == Sector::OK;
            });
        unsigned badSectors = totalSectors - goodSectors;

        result.colour = ImGuiExt::GetCustomColorU32(
            ((goodSectors == totalSectors) && goodSectors && totalSectors)
                ? ImGuiCustomCol_LoggerInfo
                : ImGuiCustomCol_LoggerError);

        result.tooltip = fmt::format(
            "c{}h{}\n{} sectors read\n{} good "
            "sectors\n{} bad sectors",
            physicalCylinder,
            physicalHead,
            totalSectors,
            goodSectors,
            badSectors);
    }
    return result;
}

void SummaryView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();
    auto [minCylinder, maxCylinder, minHead, maxHead] =
        Datastore::getDiskPhysicalBounds();
    const auto& physicalCylinderLayouts =
        Datastore::getPhysicalCylinderLayouts();
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

        ImGuiExt::TextFormattedCenteredHorizontal("Physical map (what the drive sees)");

        auto originalFontSize = ImGui::GetFontSize();
        if (ImGui::BeginTable("physicalMap",
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
                    auto [tooltip, colour] =
                        analyseTrack(diskFlux, cylinder, head);
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
                    ImGui::SetItemTooltip("%s", tooltip.c_str());
                }
            }
        }

        ImGuiExt::TextFormattedCenteredHorizontal("Logical map (what the disk image sees)");

        if (ImGui::BeginTable("logicalMap",
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

            for (unsigned physicalHead = minHead; physicalHead <= maxHead;
                physicalHead++)
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableNextColumn();

                /* Grotty code to find the first track on this head, so we can
                 * get the layout. */

                std::shared_ptr<const TrackInfo> sampleTrackInfo = nullptr;
                for (auto& [physicalLocation, trackInfo] :
                    physicalCylinderLayouts)
                    if (physicalLocation.head == physicalHead)
                    {
                        sampleTrackInfo = trackInfo;
                        break;
                    }

                if (sampleTrackInfo)
                {
                    auto text =
                        fmt::format("h{}", sampleTrackInfo->logicalHead);
                    auto textSize = ImGui::CalcTextSize(text.c_str());
                    ImGui::SetCursorPos(
                        {ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2 -
                                textSize.x / 2,
                            ImGui::GetCursorPosY() + rowHeight / 2 -
                                textSize.y / 2});
                    ImGui::Text("%s", text.c_str());

                    for (unsigned physicalCylinder = minCylinder;
                        physicalCylinder <= maxCylinder;
                        physicalCylinder++)
                    {
                        ImGui::TableNextColumn();

                        auto it = physicalCylinderLayouts.find(
                            {physicalCylinder, physicalHead});
                        if (it != physicalCylinderLayouts.end())
                        {
                            auto [tooltip, colour] = analyseTrack(
                                diskFlux, physicalCylinder, physicalHead);

                            ImGui::PushStyleColor(ImGuiCol_Header, colour);
                            ON_SCOPE_EXIT
                            {
                                ImGui::PopStyleColor();
                            };

                            auto& trackInfo = it->second;
                            float width = ImGui::GetContentRegionAvail().x *
                                              trackInfo->groupSize +
                                          ImGui::GetStyle().CellPadding.x *
                                              (trackInfo->groupSize - 1);
                            if (ImGui::Selectable(
                                    fmt::format("##logical_c{}h{}",
                                        trackInfo->logicalCylinder,
                                        trackInfo->logicalHead)
                                        .c_str(),
                                    true,
                                    ImGuiSelectableFlags_None,
                                    {width, rowHeight}))
                                Events::SeekToTrackViaPhysicalLocation::post(
                                    CylinderHead{trackInfo->physicalCylinder,
                                        trackInfo->physicalHead});

                            ImGui::PushFont(NULL, originalFontSize);
                            ON_SCOPE_EXIT
                            {
                                ImGui::PopFont();
                            };
                            ImGui::SetItemTooltip("%s", tooltip.c_str());
                        }
                    }
                }
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (unsigned physicalCylinder = minCylinder;
                physicalCylinder <= maxCylinder;
                physicalCylinder++)
            {
                ImGui::TableNextColumn();

                for (auto& [physicalLocation, trackInfo] :
                    physicalCylinderLayouts)
                {
                    if (trackInfo->physicalCylinder == physicalCylinder)
                    {
                        auto text =
                            fmt::format("c{}", trackInfo->logicalCylinder);
                        ImGui::SetCursorPosX(
                            ImGui::GetCursorPosX() +
                            ImGui::GetColumnWidth() / 2 -
                            ImGui::CalcTextSize(text.c_str()).x / 2);
                        ImGui::Text("%s", text.c_str());
                        break;
                    }
                }
            }
        }
    }
}
