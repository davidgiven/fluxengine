#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "globals.h"
#include "imageview.h"
#include "datastore.h"

using namespace hex;

ImageView::ImageView():
    View::Window("fluxengine.view.image.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
}

void ImageView::drawContent()
{
    if (!Datastore::isConfigurationValid())
        return;
    auto diskFlux = Datastore::getDiskFlux();
    if (!diskFlux)
        return;
    auto& image = diskFlux->image;
    if (!image)
        return;

    auto [minCylinder, maxCylinder, minHead, maxHead] =
        Datastore::getDiskPhysicalBounds();

    unsigned minSector = UINT_MAX;
    unsigned maxSector = 0;
    for (int i = 0; i < image->getBlockCount(); i++)
    {
        auto [logicalCylinder, logicalHead, logicalSector] =
            image->findBlock(i);
        minSector = std::min(minSector, logicalSector);
        maxSector = std::max(maxSector, logicalSector);
    }
    unsigned sectorCount = maxSector - minSector + 1;

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
            sectorCount + 2,
            ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_HighlightHoveredColumn |
                ImGuiTableFlags_NoClip | ImGuiTableFlags_NoPadInnerX |
                ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Borders))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndTable();
        };

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1, 1});
        ON_SCOPE_EXIT
        {
            ImGui::PopStyleVar();
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

        ImGui::PushStyleVar(
            ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
        ON_SCOPE_EXIT
        {
            ImGui::PopStyleVar();
        };

        float rowHeight = originalFontSize * 0.6 * 1.3;
        ImGui::TableSetupColumn(
            "", ImGuiTableColumnFlags_WidthFixed, originalFontSize * 2.0);
        ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);

        ImGui::TableNextColumn();
        for (int sector = minSector; sector <= maxSector; sector++)
        {
            ImGui::TableNextColumn();

            auto text = fmt::format("s{}", sector);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 ImGui::GetColumnWidth() / 2 -
                                 ImGui::CalcTextSize(text.c_str()).x / 2);
            ImGui::Text("%s", text.c_str());
        }

        for (unsigned physicalCylinder = minCylinder;
            physicalCylinder <= maxCylinder;
            physicalCylinder++)
            for (unsigned physicalHead = minHead; physicalHead <= maxHead;
                physicalHead++)
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                ImGui::TableNextColumn();

                {
                    auto text =
                        fmt::format("c{}h{}", physicalCylinder, physicalHead);
                    auto textSize = ImGui::CalcTextSize(text.c_str());
                    ImGui::SetCursorPos(
                        {ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2 -
                                textSize.x / 2,
                            ImGui::GetCursorPosY() + rowHeight / 2 -
                                textSize.y / 2});
                    ImGui::Text("%s", text.c_str());
                }

                for (unsigned sectorId = minSector; sectorId <= maxSector;
                    sectorId++)
                {
                    ImGui::TableNextColumn();
                    auto sector = Datastore::findSectorByPhysicalLocation(
                        {physicalCylinder, physicalHead, sectorId});

                    if (sector)
                    {
                        auto colour = ImGuiExt::GetCustomColorU32(
                            (sector->status == Sector::OK)
                                ? ImGuiCustomCol_LoggerInfo
                                : ImGuiCustomCol_LoggerError);

                        ImGui::PushStyleColor(ImGuiCol_Header, colour);
                        ON_SCOPE_EXIT
                        {
                            ImGui::PopStyleColor();
                        };

                        auto block = Datastore::findBlockByLogicalLocation(
                            {sector->logicalCylinder,
                                sector->logicalHead,
                                sector->logicalSector});

                        auto id = block.has_value() ? fmt::format("#{}", *block)
                                                    : "???";
                        if (ImGui::Selectable(fmt::format("{}##image_c{}h{}s{}",
                                                  id,
                                                  physicalCylinder,
                                                  physicalHead,
                                                  sectorId)
                                                  .c_str(),
                                true,
                                ImGuiSelectableFlags_None,
                                {0,
                                    rowHeight -
                                        ImGui::GetStyle().CellPadding.y * 2}))
                            Events::SeekToPhysicalLocationInImage::post(
                                CylinderHeadSector{
                                    physicalCylinder, physicalHead, sectorId});

                        ImGui::PushFont(NULL, originalFontSize);
                        ON_SCOPE_EXIT
                        {
                            ImGui::PopFont();
                        };
                        ImGui::SetItemTooltip(
                            fmt::format("Physical: c{}h{}s{}\n"
                                        "Logical: c{}h{}s{}\n"
                                        "Size: {} bytes\n"
                                        "Status: {}",
                                physicalCylinder,
                                physicalHead,
                                sectorId,
                                sector->logicalCylinder,
                                sector->logicalHead,
                                sectorId,
                                sector->data.size(),
                                Sector::statusToString(sector->status))
                                .c_str());
                    }
                }
            }
    }
}
