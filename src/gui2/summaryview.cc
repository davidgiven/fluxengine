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

static DiskActivityType currentDiskActivityType = DiskActivityType::None;
static unsigned currentPhysicalCylinder;
static unsigned currentPhysicalHead;
static std::string currentStatusMessage;
static OperationState currentOperationState = OperationState::Running;

static std::string getActivityLabel(DiskActivityType type)
{
    switch (type)
    {
        case DiskActivityType::None:
            return "";
        case DiskActivityType::Read:
            return "R";
        case DiskActivityType::Write:
            return "W";
    }
}

SummaryView::SummaryView():
    View::Window("fluxengine.view.summary.name", ICON_VS_COMPASS)
{
    Events::DiskActivityNotification::subscribe(
        [](DiskActivityType type, unsigned cylinder, unsigned head)
        {
            currentDiskActivityType = type;
            currentPhysicalCylinder = cylinder;
            currentPhysicalHead = head;
        });

    Events::OperationStart::subscribe(
        [](std::string message)
        {
            currentStatusMessage = message;
            currentOperationState = OperationState::Running;
        });

    Events::OperationStop::subscribe(
        [](OperationState state)
        {
            currentOperationState = state;
        });
}

static std::set<std::shared_ptr<const Sector>> findSectors(
    const DecodedDisk& diskFlux,
    unsigned physicalCylinder,
    unsigned physicalHead)
{
    std::set<std::shared_ptr<const Sector>> sectors;

    auto [startIt, endIt] = diskFlux.sectorsByPhysicalLocation.equal_range(
        {physicalCylinder, physicalHead});
    for (auto it = startIt; it != endIt; it++)
        sectors.insert(it->second);

    return sectors;
}

struct TrackAnalysis
{
    std::string tooltip;
    uint32_t colour;
};

static TrackAnalysis analyseTrack(const DecodedDisk& diskFlux,
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

static void drawCylinderRule(unsigned min, unsigned max)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    for (int cylinder = min; cylinder <= max; cylinder++)
    {
        ImGui::TableNextColumn();

        auto text = fmt::format("c{}", cylinder);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             ImGui::GetColumnWidth() / 2 -
                             ImGui::CalcTextSize(text.c_str()).x / 2);
        ImGui::Text("%s", text.c_str());
    }
}

static void drawPhysicalMap(unsigned minPhysicalCylinder,
    unsigned maxPhysicalCylinder,
    unsigned minPhysicalHead,
    unsigned maxPhysicalHead,
    const DecodedDisk& diskFlux)
{
    int numPhysicalCylinders = maxPhysicalCylinder - minPhysicalCylinder + 1;
    int numPhysicalHeads = maxPhysicalHead - minPhysicalHead + 1;

    auto originalFontSize = ImGui::GetFontSize();
    if (ImGui::BeginTable("physicalMap",
            numPhysicalCylinders + 1,
            ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_HighlightHoveredColumn |
                ImGuiTableFlags_NoClip | ImGuiTableFlags_NoPadInnerX |
                ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Borders))
    {
        DEFER(ImGui::EndTable());

        ImGui::PushFont(NULL, originalFontSize * 0.6);
        DEFER(ImGui::PopFont());

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        DEFER(ImGui::PopStyleVar());

        float rowHeight = originalFontSize * 0.6 * 2.0;
        ImGui::TableSetupColumn(
            "", ImGuiTableColumnFlags_WidthFixed, originalFontSize);

        drawCylinderRule(minPhysicalCylinder, maxPhysicalCylinder);

        for (unsigned head = minPhysicalHead; head <= maxPhysicalHead; head++)
        {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
            ImGui::TableNextColumn();

            auto text = fmt::format("h{}", head);
            auto textSize = ImGui::CalcTextSize(text.c_str());
            ImGui::SetCursorPos(
                {ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2 -
                        textSize.x / 2,
                    ImGui::GetCursorPosY() + rowHeight / 2 - textSize.y / 2});
            ImGui::Text("%s", text.c_str());

            ImGui::PushStyleVar(
                ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
            DEFER(ImGui::PopStyleVar());

            for (unsigned cylinder = minPhysicalCylinder;
                cylinder <= maxPhysicalCylinder;
                cylinder++)
            {
                auto [tooltip, colour] = analyseTrack(diskFlux, cylinder, head);
                ImGui::PushStyleColor(ImGuiCol_Header, colour);
                DEFER(ImGui::PopStyleColor());
                ImGui::PushFont(NULL, originalFontSize);
                DEFER(ImGui::PopFont());

                ImGui::TableNextColumn();
                std::string diskActivityLabel = "";
                if ((currentDiskActivityType != DiskActivityType::None) &&
                    (cylinder == currentPhysicalCylinder) &&
                    (head == currentPhysicalHead))
                    diskActivityLabel =
                        getActivityLabel(currentDiskActivityType);

                if (ImGui::Selectable(
                        fmt::format(
                            "{}##c{}h{}", diskActivityLabel, cylinder, head)
                            .c_str(),
                        true,
                        ImGuiSelectableFlags_None,
                        {0, rowHeight}))
                    Events::SeekToTrackViaPhysicalLocation::post(
                        CylinderHead{cylinder, head});

                ImGui::SetItemTooltip("%s", tooltip.c_str());
            }
        }
    }
}

static void drawLogicalMap(unsigned minPhysicalCylinder,
    unsigned maxPhysicalCylinder,
    unsigned minPhysicalHead,
    unsigned maxPhysicalHead,
    const DecodedDisk& diskFlux,
    const DiskLayout& diskLayout)
{
    auto originalFontSize = ImGui::GetFontSize();
    int numPhysicalCylinders = maxPhysicalCylinder - minPhysicalCylinder + 1;
    int numPhysicalHeads = maxPhysicalHead - minPhysicalHead + 1;

    if (ImGui::BeginTable("logicalMap",
            numPhysicalCylinders + 1,
            ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_HighlightHoveredColumn |
                ImGuiTableFlags_NoClip | ImGuiTableFlags_NoPadInnerX |
                ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Borders))
    {
        DEFER(ImGui::EndTable());

        ImGui::PushFont(NULL, originalFontSize * 0.6);
        DEFER(ImGui::PopFont());

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        DEFER(ImGui::PopStyleVar());

        float rowHeight = originalFontSize * 0.6 * 2.0;
        ImGui::TableSetupColumn(
            "", ImGuiTableColumnFlags_WidthFixed, originalFontSize);

        for (unsigned physicalHead = minPhysicalHead;
            physicalHead <= maxPhysicalHead;
            physicalHead++)
        {
            unsigned logicalHead =
                diskLayout.remapHeadPhysicalToLogical(physicalHead);

            ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
            ImGui::TableNextColumn();

            auto text = fmt::format("h{}", logicalHead);
            auto textSize = ImGui::CalcTextSize(text.c_str());
            ImGui::SetCursorPos(
                {ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2 -
                        textSize.x / 2,
                    ImGui::GetCursorPosY() + rowHeight / 2 - textSize.y / 2});
            ImGui::Text("%s", text.c_str());

            ImGui::PushStyleVar(
                ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
            DEFER(ImGui::PopStyleVar());

            for (unsigned physicalCylinder = minPhysicalCylinder;
                physicalCylinder <= maxPhysicalCylinder;
                physicalCylinder++)
            {
                ImGui::TableNextColumn();

                auto& ptl = diskLayout.layoutByPhysicalLocation.at(
                    {physicalCylinder, physicalHead});
                if (ptl->groupOffset == 0)
                {
                    auto [tooltip, colour] =
                        analyseTrack(diskFlux, physicalCylinder, physicalHead);

                    ImGui::PushStyleColor(ImGuiCol_Header, colour);
                    DEFER(ImGui::PopStyleColor());
                    ImGui::PushFont(NULL, originalFontSize);
                    DEFER(ImGui::PopFont());

                    std::string diskActivityLabel = "";
                    if (currentDiskActivityType != DiskActivityType::None)
                    {
                        auto ptlactive = diskLayout.layoutByPhysicalLocation.at(
                            {currentPhysicalCylinder, currentPhysicalHead});
                        if (ptlactive->logicalTrackLayout ==
                            ptl->logicalTrackLayout)
                            diskActivityLabel =
                                getActivityLabel(currentDiskActivityType);
                    }

                    float width = ImGui::GetContentRegionAvail().x *
                                      diskLayout.groupSize +
                                  ImGui::GetStyle().CellPadding.x *
                                      (diskLayout.groupSize - 1);
                    if (ImGui::Selectable(
                            fmt::format("{}##logical_c{}h{}",
                                diskActivityLabel,
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

                    ImGui::SetItemTooltip("%s", tooltip.c_str());
                }
            }
        }

        drawCylinderRule(minPhysicalCylinder, maxPhysicalCylinder);
    }
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

    if (diskFlux)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1, 1});
        DEFER(ImGui::PopStyleVar());

        auto backgroundColour = ImGui::GetColorU32(ImGuiCol_WindowBg);
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, backgroundColour);
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, backgroundColour);
        DEFER(ImGui::PopStyleColor(2));

        ImGuiExt::TextFormattedCenteredHorizontal(
            "fluxengine.view.summary.physical"_lang);

        drawPhysicalMap(minPhysicalCylinder,
            maxPhysicalCylinder,
            minPhysicalHead,
            maxPhysicalHead,
            *diskFlux);

        ImGuiExt::TextFormattedCenteredHorizontal(
            "fluxengine.view.summary.logical"_lang);

        drawLogicalMap(minPhysicalCylinder,
            maxPhysicalCylinder,
            minPhysicalHead,
            maxPhysicalHead,
            *diskFlux,
            *diskLayout);

        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize()));

        std::string message = currentStatusMessage;
        if (!message.empty())
        {
            switch (currentOperationState)
            {
                case OperationState::Running:
                    message += (std::
                            string) "fluxengine.view.status.runningSuffix"_lang;
                    break;

                case OperationState::Succeeded:
                    message += (std::
                            string) "fluxengine.view.status.succeededSuffix"_lang;
                    break;

                case OperationState::Failed:
                    message += (std::
                            string) "fluxengine.view.status.failedSuffix"_lang;
                    break;
            }
            ImGuiExt::TextFormattedCenteredHorizontal(message);
        }
    }
}
