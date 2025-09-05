#include <hex/api/content_registry/user_interface.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include "customview.h"

using namespace hex;

CustomView::CustomView():
    View::Window("fluxengine.view.custom.name", ICON_VS_DEBUG_LINE_BY_LINE)
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

void CustomView::drawContent()
{
}
