#pragma once

#include <hex/helpers/logger.hpp>
#include <hex/api/localization_manager.hpp>
#include <hex/api/event_manager.hpp>

class CylinderHeadSector;

extern const hex::UnlocalizedString FLUXENGINE_CONFIG;

namespace ImGui
{
    static inline void Text(std::string text)
    {
        ImGui::Text(text.c_str());
    }
}

namespace Events
{
    using namespace hex;
    EVENT_DEF(SeekToPhysicalLocationInImage, CylinderHeadSector);
}
