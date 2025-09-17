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

template <typename K, typename V>
inline const V& findOrDefault(
    const std::map<K, V>& map, const K& key, const V& defaultValue = V())
{
    auto it = map.find(key);
    if (it == map.end())
        return defaultValue;
    return it->second;
}
