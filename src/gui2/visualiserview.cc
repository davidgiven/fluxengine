#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/disk.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "visualiserview.h"
#include "datastore.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>
#include <algorithm>

using namespace hex;

static constexpr float INNER_RADIUS = 50;
static constexpr float SEGMENTS_PER_RADIAN = 24;

VisualiserView::VisualiserView():
    View::Window("fluxengine.view.visualiser.name", ICON_VS_PREVIEW)
{
}

void VisualiserView::drawContent()
{
    const auto& diskLayout = Datastore::getDiskLayout();
    const auto& disk = Datastore::getDisk();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorScreenPos();
    auto size = ImGui::GetContentRegionAvail();
    auto padding = ImGui::GetStyle().WindowPadding;
    uint32_t fg = ImGui::GetColorU32(ImGuiCol_Text);
    uint32_t bg = ImGui::GetColorU32(ImGuiCol_WindowBg);
    uint32_t diskbg = ImGui::GetColorU32(ImGuiCol_FrameBg);

    ImVec2 centre(pos.x + size.x / 2, pos.y + size.y / 2);
    float outerRadius = (size.x - padding.x * 2) / 2;
    ImVec2 side0pos(centre.x, centre.y - outerRadius - padding.y);
    ImVec2 side1pos(centre.x, centre.y + outerRadius + padding.y);

    bool badData = false;

    auto drawCentered =
        [&](const ImVec2& pos, uint32_t colour, const std::string& s)
    {
        auto size = ImGui::CalcTextSize(s.c_str());
        drawList->AddText({pos.x - size.x / 2, pos.y - size.y / 2},
            colour,
            &*s.begin(),
            &*s.end());
    };

    auto drawSide = [&](int head, ImVec2 pos)
    {
        drawList->AddCircleFilled(pos, outerRadius, diskbg, 0);
        drawList->AddCircleFilled(pos, INNER_RADIUS, bg, 0);
        drawList->AddCircle(pos, outerRadius, fg, 0);
        drawList->AddCircle(pos, INNER_RADIUS, fg, 0);
        drawCentered(pos, fg, fmt::format("h{}", head));

        if (!diskLayout || !disk)
            return;

        int numPhysicalTracks =
            diskLayout->maxPhysicalCylinder - diskLayout->minPhysicalCylinder;
        float trackSpacing = (float)(outerRadius - INNER_RADIUS) /
                             (float)(numPhysicalTracks + 2);

        for (const auto& [ch, track] : disk->tracksByPhysicalLocation)
        {
            if (ch.head != head)
                continue;
            if (!track->fluxmap)
                continue;

            const auto& indexMarks = track->fluxmap->getIndexMarks();
            nanoseconds_t rotationalPeriod;
            if (indexMarks.empty())
            {
                badData = true;
                continue;
            }
            if (indexMarks.size() >= 2)
                rotationalPeriod = indexMarks[1] - indexMarks[0];
            else
            {
                rotationalPeriod = disk->rotationalPeriod;
                if (rotationalPeriod == 0)
                {
                    badData = true;
                    continue;
                }
            }
            float radiansPerNano = IM_PI * 2.0 / rotationalPeriod;

            float radius =
                (float)outerRadius - ((float)ch.cylinder + 0.5) * trackSpacing;

            auto normalisePosition = [&](nanoseconds_t timestamp)
            {
                if (timestamp < indexMarks[0])
                    return timestamp - (indexMarks[0] - rotationalPeriod);

                auto it = std::upper_bound(
                    indexMarks.begin(), indexMarks.end(), timestamp);
                it--;
                return timestamp - *it;
            };

            auto drawArcRadians =
                [&](float startRadians, float endRadians, uint32_t colour)
            {
                int segments =
                    ImClamp((endRadians - startRadians) * SEGMENTS_PER_RADIAN,
                        6.0F,
                        SEGMENTS_PER_RADIAN * IM_PI * 2);
                drawList->PathArcTo(pos,
                    radius,
                    startRadians - IM_PI / 2,
                    endRadians - IM_PI / 2,
                    segments);
                drawList->PathStroke(
                    colour, ImDrawFlags_None, trackSpacing * 0.75);
            };

            auto drawArcNs =
                [&](nanoseconds_t startNs, nanoseconds_t endNs, uint32_t colour)
            {
                float startRadians =
                    normalisePosition(startNs) * radiansPerNano;
                float endRadians = normalisePosition(endNs) * radiansPerNano;
                drawArcRadians(startRadians, endRadians, colour);
            };

            for (const auto& sector : track->normalisedSectors)
            {
                if (sector->headerStartTime && sector->headerEndTime)
                    drawArcNs(sector->headerStartTime,
                        sector->headerEndTime,
                        ImGuiExt::GetCustomColorU32(
                            ImGuiCustomCol_AdvancedEncodingMultiChar));
                if (sector->dataStartTime && sector->dataEndTime)
                    drawArcNs(sector->dataStartTime,
                        sector->dataEndTime,
                        ImGuiExt::GetCustomColorU32(
                            sector->status == Sector::OK
                                ? ImGuiCustomCol_AdvancedEncodingASCII
                                : ImGuiCustomCol_DiffRemoved));
            }
            if (track->normalisedSectors.empty())
                drawArcRadians(0,
                    IM_PI * 2,
                    ImGuiExt::GetCustomColorU32(ImGuiCustomCol_DiffRemoved));
        }

        drawList->AddLine({pos.x, pos.y - INNER_RADIUS},
            {pos.x, pos.y - outerRadius},
            ImGui::GetColorU32(ImGuiCol_PlotHistogram),
            1.0);
    };

    drawSide(0, side0pos);
    drawSide(1, side1pos);

    if (badData)
        ImGuiExt::TextOverlay("fluxengine.view.visualiser.missingData"_lang,
            pos + ImVec2(size.x / 2, size.y - 100),
            size.x * 0.7);
}
