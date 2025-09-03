#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/flux.h"
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

void SummaryView::drawContent()
{
    auto diskFlux = Datastore::getDiskFlux();

    if (ImGui::Button("fluxengine.summary.controls.read"_lang))
        Datastore::beginRead();

    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::Button("fluxengine.summary.controls.write"_lang);
    ImGui::EndDisabled();
}
