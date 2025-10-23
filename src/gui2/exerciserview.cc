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
#include "exerciserview.h"
#include "datastore.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

ExerciserView::ExerciserView():
    View::Modal("fluxengine.view.exerciser.name", ICON_VS_DEBUG)
{
}

void ExerciserView::drawContent()
{
    static int selectedDrive = 0;
    static int selectedTrack = 0;

    const float label_width = ImGui::GetFontSize() * 6;
    ImGui::PushItemWidth(-label_width);
    DEFER(ImGui::PopItemWidth());

    ImGui::SliderInt("fluxengine.view.exerciser.drive"_lang,
        &selectedDrive,
        0,
        1,
        "%d",
        ImGuiSliderFlags_None);
    ImGui::SliderInt("fluxengine.view.exerciser.cylinder"_lang,
        &selectedTrack,
        0,
        82,
        "%d",
        ImGuiSliderFlags_None);

    if (ImGui::BeginTable("nudgeTable",
            3,
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp,
            {ImGui::GetContentRegionAvail().x - label_width, 0}))
    {
        DEFER(ImGui::EndTable());

        ImGui::TableNextColumn();
        if (ImGui::Button("fluxengine.view.exerciser.nudgeDown"_lang,
                {ImGui::GetContentRegionAvail().x, 0}))
            selectedTrack--;
        ImGui::TableNextColumn();
        if (ImGui::Button("fluxengine.view.exerciser.reset"_lang,
                {ImGui::GetContentRegionAvail().x, 0}))
            selectedTrack = 0;
        ImGui::TableNextColumn();
        if (ImGui::Button("fluxengine.view.exerciser.nudgeUp"_lang,
                {ImGui::GetContentRegionAvail().x, 0}))
            selectedTrack++;
    }

    selectedTrack = std::clamp(selectedTrack, 0, 82);
}