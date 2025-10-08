#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/decoded.h"
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

static std::set<std::shared_ptr<const Sector>> findSectors(
    std::shared_ptr<const DecodedDisk>& diskFlux,
    unsigned physicalCylinder,
    unsigned physicalHead)
{
    std::set<std::shared_ptr<const Sector>> sectors;

    if (diskFlux)
    {
        auto [startIt, endIt] = diskFlux->sectorsByPhysicalLocation.equal_range(
            {physicalCylinder, physicalHead});
        for (auto it = startIt; it != endIt; it++)
            sectors.insert(it->second);
    }

    return sectors;
}

struct TrackAnalysis
{
    std::string tooltip;
    uint32_t colour;
};

TrackAnalysis analyseTrack(std::shared_ptr<const DecodedDisk>& diskFlux,
    unsigned physicalCylinder,
    unsigned physicalHead)
{
    TrackAnalysis result = {};
    auto sectors = findSectors(diskFlux, physicalCylinder, physicalHead);
    result.colour = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    result.tooltip = "No data";
    if (!sectors.empty())
    {
        unsigned totalSectors = sectors.size();
        unsigned goodSectors = std::ranges::count_if(sectors,
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
    auto diskFlux = Datastore::getDecodedDisk();
    auto diskLayout = Datastore::getDiskLayout();
    if (!diskFlux || !diskLayout)
        return;

    auto [minPhysicalCylinder,
        maxPhysicalCylinder,
        minPhysicalHead,
        maxPhysicalHead] = diskLayout->getPhysicalBounds();
    int numPhysicalCylinders = maxPhysicalCylinder - minPhysicalCylinder + 1;
    int numPhysicalHeads = maxPhysicalHead - minPhysicalHead + 1;

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

        ImGuiExt::TextFormattedCenteredHorizontal(
            "fluxengine.view.summary.physical"_lang);

        auto originalFontSize = ImGui::GetFontSize();
        if (ImGui::BeginTable("physicalMap",
                numPhysicalCylinders + 1,
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
            for (int cylinder = minPhysicalCylinder;
                cylinder <= maxPhysicalCylinder;
                cylinder++)
            {
                ImGui::TableNextColumn();

                auto text = fmt::format("c{}", cylinder);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     ImGui::GetColumnWidth() / 2 -
                                     ImGui::CalcTextSize(text.c_str()).x / 2);
                ImGui::Text("%s", text.c_str());
            }

            for (unsigned head = minPhysicalHead; head <= maxPhysicalHead;
                head++)
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

                for (unsigned cylinder = minPhysicalCylinder;
                    cylinder <= maxPhysicalCylinder;
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

        ImGuiExt::TextFormattedCenteredHorizontal(
            "fluxengine.view.summary.logical"_lang);

        /* Must match the physicalMap table width above. */
        if (ImGui::BeginTable("logicalMap",
                numPhysicalCylinders + 1,
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

            for (unsigned physicalHead = minPhysicalHead;
                physicalHead <= maxPhysicalHead;
                physicalHead++)
            {
                unsigned logicalHead =
                    diskLayout->remapHeadPhysicalToLogical(physicalHead);

                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableNextColumn();

                auto text = fmt::format("h{}", logicalHead);
                auto textSize = ImGui::CalcTextSize(text.c_str());
                ImGui::SetCursorPos({ImGui::GetCursorPosX() +
                                         ImGui::GetColumnWidth() / 2 -
                                         textSize.x / 2,
                    ImGui::GetCursorPosY() + rowHeight / 2 - textSize.y / 2});
                ImGui::Text("%s", text.c_str());

                for (unsigned physicalCylinder = minPhysicalCylinder;
                    physicalCylinder <= maxPhysicalCylinder;
                    physicalCylinder++)
                {
                    ImGui::TableNextColumn();

                    auto& ptl = diskLayout->layoutByPhysicalLocation.at(
                        {physicalCylinder, physicalHead});
                    if (ptl->groupOffset == 0)
                    {
                        auto [tooltip, colour] = analyseTrack(
                            diskFlux, physicalCylinder, physicalHead);

                        ImGui::PushStyleColor(ImGuiCol_Header, colour);
                        ON_SCOPE_EXIT
                        {
                            ImGui::PopStyleColor();
                        };

                        float width = ImGui::GetContentRegionAvail().x *
                                          diskLayout->groupSize +
                                      ImGui::GetStyle().CellPadding.x *
                                          (diskLayout->groupSize - 1);
                        if (ImGui::Selectable(
                                fmt::format("##logical_c{}h{}",
                                    ptl->logicalTrackLayout->logicalCylinder,
                                    ptl->logicalTrackLayout->logicalHead)
                                    .c_str(),
                                true,
                                ImGuiSelectableFlags_None,
                                {width, rowHeight}))
                            Events::SeekToTrackViaPhysicalLocation::post(
                                CylinderHead{
                                    ptl->logicalTrackLayout->physicalCylinder,
                                    ptl->logicalTrackLayout->physicalHead});

                        ImGui::PushFont(NULL, originalFontSize);
                        ON_SCOPE_EXIT
                        {
                            ImGui::PopFont();
                        };
                        ImGui::SetItemTooltip("%s", tooltip.c_str());
                    }
                }
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (unsigned physicalCylinder = minPhysicalCylinder;
                physicalCylinder <= maxPhysicalCylinder;
                physicalCylinder++)
            {
                ImGui::TableNextColumn();

                for (auto& [ch, ptl] : diskLayout->layoutByPhysicalLocation)
                {
                    if (ptl->logicalTrackLayout->physicalCylinder ==
                        physicalCylinder)
                    {
                        auto text = fmt::format(
                            "c{}", ptl->logicalTrackLayout->logicalCylinder);
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
