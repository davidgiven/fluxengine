#pragma once

namespace ImGui
{
    static inline void Text(std::string text)
    {
        ImGui::Text(text.c_str());
    }
}

