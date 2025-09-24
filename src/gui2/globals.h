#pragma once

#include <hex/helpers/logger.hpp>
#include <hex/api/localization_manager.hpp>
#include <hex/api/event_manager.hpp>

class CylinderHeadSector;
class CylinderHead;

static const std::string FLUXENGINE_CONFIG = "fluxengine.config";

namespace ImGui
{
    static inline void Text(std::string text)
    {
        ImGui::Text("%s", text.c_str());
    }
}

namespace Events
{
    using namespace hex;
    EVENT_DEF(SeekToSectorViaPhysicalLocation, CylinderHeadSector);
    EVENT_DEF(SeekToTrackViaPhysicalLocation, CylinderHead);
}
