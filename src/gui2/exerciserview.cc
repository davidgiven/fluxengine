#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/api/task_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/layout.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "exerciserview.h"
#include "datastore.h"
#include "utils.h"
#include "lib/usb/usb.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

static DiskLayout::LayoutBounds physicalBounds = {-1, -1, -1, -1};
static int selectedDrive;
static int selectedHead;
static int selectedCylinder;

ExerciserView::ExerciserView():
    View::Modal("fluxengine.view.exerciser.name", ICON_VS_DEBUG)
{
}

void ExerciserView::onOpen()
{
    Datastore::reset();
    Datastore::runOnWorkerThread(
        []
        {
            auto tracks =
                parseCylinderHeadsString(globalConfig()->drive().tracks());
            auto bounds = DiskLayout::getBounds(std::views::all(tracks));
            hex::TaskManager::doLaterOnce(
                [=]
                {
                    ::physicalBounds = bounds;
                    selectedDrive = 0;
                    selectedHead = bounds.minHead;
                    selectedCylinder = bounds.minCylinder;
                });
        });
}

void ExerciserView::onClose()
{
    physicalBounds.minCylinder = physicalBounds.maxCylinder =
        physicalBounds.minHead = physicalBounds.maxHead = -1;
}

void ExerciserView::drawContent()
{
    if (physicalBounds.minCylinder == -1)
        return;

    const float label_width = ImGui::GetFontSize() * 6;
    ImGui::PushItemWidth(-label_width);
    DEFER(ImGui::PopItemWidth());

    int oldCylinder = selectedCylinder;

    ImGui::SliderInt("fluxengine.view.exerciser.drive"_lang,
        &selectedDrive,
        0,
        1,
        "%d",
        ImGuiSliderFlags_None);
    ImGui::SliderInt("fluxengine.view.exerciser.cylinder"_lang,
        &selectedCylinder,
        physicalBounds.minCylinder,
        physicalBounds.maxCylinder,
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
            selectedCylinder--;
        ImGui::TableNextColumn();
        if (ImGui::Button("fluxengine.view.exerciser.reset"_lang,
                {ImGui::GetContentRegionAvail().x, 0}))
            selectedCylinder = 0;
        ImGui::TableNextColumn();
        if (ImGui::Button("fluxengine.view.exerciser.nudgeUp"_lang,
                {ImGui::GetContentRegionAvail().x, 0}))
            selectedCylinder++;
    }

    selectedCylinder = std::clamp(selectedCylinder,
        physicalBounds.minCylinder,
        physicalBounds.maxCylinder);

    if (selectedCylinder != oldCylinder)
    {
        Datastore::runOnWorkerThread(
            [=]
            {
                usbSetDrive(
                    selectedDrive, false, globalConfig()->drive().index_mode());
                usbSeek(selectedCylinder);
            });
    }
}