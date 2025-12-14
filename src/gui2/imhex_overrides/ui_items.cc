#define addTitleBarButtons addTitleBarButtons_disabled
#include "../../../dep/imhex/plugins/builtin/source/content/ui_items.cpp"
#undef addTitleBarButtons

namespace hex::plugin::builtin
{
    void addTitleBarButtons()
    {
        if (dbg::debugModeEnabled())
        {
            ContentRegistry::UserInterface::addTitleBarButton(ICON_VS_DEBUG,
                ImGuiCustomCol_ToolbarGray,
                "hex.builtin.title_bar_button.debug_build",
                []
                {
                    RequestOpenPopup::post("DebugMenu");
                });
        }

        ContentRegistry::UserInterface::addTitleBarButton(ICON_VS_SMILEY,
            ImGuiCustomCol_ToolbarGray,
            "hex.builtin.title_bar_button.feedback",
            []
            {
                hex::openWebpage(
                    "https://github.com/davidgiven/fluxengine/discussions/"
                    "categories/general");
            });
    }
}
