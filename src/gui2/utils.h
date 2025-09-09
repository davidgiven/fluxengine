#pragma once

#include <map>
#include <string>
#include <hex/api/content_registry/settings.hpp>
#include "datastore.h"

struct ImVec2;

using OptionsMap = std::map<std::string, std::string>;

extern int MaybeDisabledButton(
    const std::string& message, const ImVec2& size, bool isDisabled);

extern OptionsMap stringToOptions(const std::string& optionsString);
extern std::string optionsToString(const OptionsMap& options);

template <typename T>
class DynamicSetting
{
public:
    DynamicSetting(
        std::string_view path, std::string_view leaf, T defaultValue):
        _key(hex::UnlocalizedString(fmt::format("{}.{}", path, leaf))),
        _defaultValue(defaultValue),
        _cachedValue()
    {
    }

    operator const T&()
    {
        if (!_cachedValue)
        {
            _cachedValue =
                std::make_optional(hex::ContentRegistry::Settings::read<T>(
                    FLUXENGINE_CONFIG, _key, _defaultValue));
        }
        return *_cachedValue;
    }

    T operator=(T value)
    {
        if (!_cachedValue || (*_cachedValue != value))
        {
            hex::ContentRegistry::Settings::write<T>(
                FLUXENGINE_CONFIG, _key, value);
            *_cachedValue = value;
            Datastore::rebuildConfiguration();
        }
        return value;
    }

private:
    const hex::UnlocalizedString _key;
    const T _defaultValue;
    std::optional<T> _cachedValue;
};
