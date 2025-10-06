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
#include "logview.h"
#include "datastore.h"
#include "utils.h"
#include <implot.h>
#include <implot_internal.h>

using namespace hex;

static std::stringstream logstream;
static std::unique_ptr<LogRenderer> logRenderer =
    LogRenderer::create(logstream);
static bool scrollToEndOnNextRedraw = false;

LogView::LogView(): View::Window("fluxengine.view.log.name", ICON_VS_DEBUG) {}

void LogView::drawContent()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x00000000);
    ON_SCOPE_EXIT
    {
        ImGui::PopStyleColor();
    };

    auto buffer = logstream.view();
    ImGui::InputTextMultiline("##logview",
        (char*)buffer.data(),
        buffer.size(),
        ImGui::GetContentRegionAvail(),
        ImGuiInputTextFlags_ReadOnly);

    /* Hacky code to scroll to the end of the window if something's changed.
     * See https://github.com/ocornut/imgui/issues/5484
     */

    if (scrollToEndOnNextRedraw)
    {
        scrollToEndOnNextRedraw = false;

        ImGuiContext& g = *GImGui;
        const char* child_window_name = NULL;
        ImFormatStringToTempBuffer(&child_window_name,
            NULL,
            "%s/%s_%08X",
            g.CurrentWindow->Name,
            "##logview",
            ImGui::GetID("##logview"));
        ImGuiWindow* child_window = ImGui::FindWindowByName(child_window_name);
        ImGui::SetScrollY(child_window, child_window->ScrollMax.y);
    }
}

void LogView::logMessage(const AnyLogMessage& message)
{
    logRenderer->add(message);
    scrollToEndOnNextRedraw = true;
}
