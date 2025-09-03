#pragma once

extern const hex::UnlocalizedString FLUXENGINE_CONFIG;

namespace ImGui
{
    static inline void Text(std::string text)
    {
        ImGui::Text(text.c_str());
    }
}

