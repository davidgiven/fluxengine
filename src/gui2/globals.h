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

enum class DiskActivityType
{
    None,
    Read,
    Write,
};

enum class OperationState
{
    Running,
    Succeeded,
    Failed
};

namespace Events
{
    using namespace hex;
    EVENT_DEF(SeekToSectorViaPhysicalLocation, CylinderHeadSector);
    EVENT_DEF(SeekToTrackViaPhysicalLocation, CylinderHead);
    EVENT_DEF(DiskActivityNotification, DiskActivityType, unsigned, unsigned);
    EVENT_DEF(OperationStart, std::string);
    EVENT_DEF(OperationStop, OperationState);
}

#define DEFER(_stmt) \
    ON_SCOPE_EXIT    \
    {                \
        _stmt;       \
    }
