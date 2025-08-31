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
    #if 0
    ImGui::Combo("hex.builtin.view.logs.log_level"_lang,
        &m_logLevel,
        "DEBUG\0INFO\0WARNING\0ERROR\0FATAL\0");
        #endif

    ImGui::Text("Hello, world!");
    if (ImGui::BeginTable("##logs",
            2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("hex.builtin.view.logs.component"_lang,
            ImGuiTableColumnFlags_WidthFixed,
            100_scaled);
        ImGui::TableSetupColumn("hex.builtin.view.logs.message"_lang);
        ImGui::TableSetupScrollFreeze(0, 1);

        ImGui::TableHeadersRow();

            #if 0
            const auto &logs = log::impl::getLogEntries();
            for (const auto &[project, level, message] : logs | std::views::reverse) {
                if (!shouldDisplay(level, m_logLevel)) {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(project.data(), project.data() + project.size());
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, getColor(level).Value);
                ImGui::TextUnformatted(message.c_str());
                ImGui::PopStyleColor();
            }
#endif

        ImGui::EndTable();
    }
}
