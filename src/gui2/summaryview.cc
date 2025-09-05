#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
#include "lib/data/sector.h"
#include "globals.h"
#include "summaryview.h"
#include "datastore.h"

using namespace hex;

SummaryView::SummaryView():
    View::Window("fluxengine.view.summary.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
    ContentRegistry::UserInterface::addMenuItem(
        {"hex.builtin.menu.extras", "fluxengine.view.custom.name"},
        ICON_VS_BRACKET_ERROR,
        2500,
        Shortcut::None,
        [&, this]
        {
            this->getWindowOpenState() = true;
        });
}

static std::optional<std::set<std::shared_ptr<const Sector>>> findSectors(
    std::shared_ptr<const DiskFlux>& diskFlux, int cylinder, int head)
{
    if (diskFlux)
        for (auto& it : diskFlux->tracks)
        {
            if ((it->trackInfo->physicalTrack == cylinder) &&
                (it->trackInfo->physicalSide == head))
                return std::make_optional(it->sectors);
        }

    return {};
}

void SummaryView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();
    auto [minCylinder, maxCylinder, minHead, maxHead] =
        Datastore::getDiskBounds();
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

            ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.6);
            ON_SCOPE_EXIT
            {
                ImGui::PopFont();
            };

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
            ON_SCOPE_EXIT
            {
                ImGui::PopStyleVar();
            };

            float rowHeight = ImGui::GetFontSize() * 2.0;
            ImGui::TableSetupColumn(
                "", ImGuiTableColumnFlags_WidthFixed, rowHeight / 2);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (int cylinder = minCylinder; cylinder <= maxCylinder;
                cylinder++)
            {
                ImGui::TableNextColumn();

                auto text = fmt::format("{}", cylinder);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     ImGui::GetColumnWidth() / 2 -
                                     ImGui::CalcTextSize(text.c_str()).x / 2);
                ImGui::Text("%s", text.c_str());
            }

            for (int head = minHead; head <= maxHead; head++)
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableNextColumn();

                auto text = fmt::format("{}", head);
                auto textSize = ImGui::CalcTextSize(text.c_str());
                ImGui::SetCursorPos({ImGui::GetCursorPosX() +
                                         ImGui::GetColumnWidth() / 2 -
                                         textSize.x / 2,
                    ImGui::GetCursorPosY() + rowHeight / 2 - textSize.y / 2});
                ImGui::Text("%s", text.c_str());

                for (int cylinder = minCylinder; cylinder <= maxCylinder;
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
                            (goodSectors == totalSectors)
                                ? ImGuiCustomCol_LoggerInfo
                                : ImGuiCustomCol_LoggerError);
                    }

                    ImGui::PushStyleColor(ImGuiCol_Header, colour);
                    ON_SCOPE_EXIT
                    {
                        ImGui::PopStyleColor();
                    };

                    ImGui::TableNextColumn();
                    ImGui::Selectable(
                        fmt::format("##c{}h{}", cylinder, head).c_str(),
                        true,
                        ImGuiSelectableFlags_None,
                        {0, rowHeight});
                    ImGui::SetItemTooltip(
                        totalSectors ? fmt::format("{} sectors read\n{} good "
                                                   "sectors\n{} bad sectors",
                                           totalSectors,
                                           goodSectors,
                                           totalSectors - goodSectors)
                                           .c_str()
                                     : "No data");
                }
            }
        }
    }

    {
        ImGui::BeginDisabled(Datastore::isBusy());
        ON_SCOPE_EXIT
        {
            ImGui::EndDisabled();
        };

        ImGui::SetCursorPosY(
            ImGui::GetWindowHeight() - ImGui::GetFontSize() * 2);
        if (ImGui::Button("fluxengine.summary.controls.read"_lang))
            Datastore::beginRead();

        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button("fluxengine.summary.controls.write"_lang);
        ImGui::EndDisabled();
    }
    {
        ImGui::BeginDisabled(!Datastore::isBusy());
        ON_SCOPE_EXIT
        {
            ImGui::EndDisabled();
        };
        ImGui::SameLine();
        if (ImGui::Button("fluxengine.summary.controls.stop"_lang))
            Datastore::stop();
    }
}
