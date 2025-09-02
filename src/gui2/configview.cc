#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/api/task_manager.hpp>
#include <fonts/vscode_icons.hpp>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/proto.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "configview.h"
#include "datastore.h"

/* Imported from src/gui/drivetypes */
extern const std::map<std::string, const ConfigProto*> drivetypes;

using namespace hex;
using OptionsMap = std::map<std::string, std::string>;

static const hex::UnlocalizedString FLUXENGINE_CONFIG("fluxengine.config");
static const hex::UnlocalizedString SELECTED_DRIVE(
    "fluxengine.config.selected_drive");
static const hex::UnlocalizedString HIGH_DENSITY(
    "fluxengine.config.high_density");

namespace
{
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

        operator T()
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
            }
            return value;
        }

    private:
        const hex::UnlocalizedString _key;
        const T _defaultValue;
        std::optional<T> _cachedValue;
    };
}

ConfigView::ConfigView():
    View::Window("fluxengine.view.config.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
}

static OptionsMap stringToOptions(const std::string& optionsString)
{
    OptionsMap result;
    for (auto it : std::views::split(optionsString, ' '))
    {
        std::string left(&*it.begin(), std::ranges::distance(it));
        std::string right;
        auto i = left.find('=');
        if (i != std::string::npos)
        {
            right = left.substr(i + 1);
            left = left.substr(0, i);
        }
        result[left] = right;
    }
    return result;
}

static std::string optionsToString(const OptionsMap& options)
{
    std::stringstream ss;
    for (auto& it : options)
    {
        if (ss.rdbuf()->in_avail())
            ss << " ";
        ss << it.first;
        if (!it.second.empty())
            ss << "=" << it.second;
    }
    return ss.str();
}

static void showOption(OptionsMap& options, const OptionProto& option)
{
    auto it = options.find(option.name());
    bool value = option.set_by_default();
    if (it != options.end())
        value = (it->second == "true");

    bool oldValue = value;
    ImGui::Checkbox(option.comment().c_str(), &value);
    if (oldValue != value)
        options[option.name()] = value ? "true" : "false";
}

static void showOptionGroup(OptionsMap& options,
    const OptionGroupProto& optionGroup,
    const char* name = nullptr)
{
    std::string value;

    if (!name)
        name = optionGroup.comment().c_str();

    if (optionGroup.has_name())
    {
        /* This is a foo=bar group. */

        auto it = options.find(optionGroup.name());
        if (it == options.end())
        {
            for (auto& it : optionGroup.option())
                if (it.set_by_default())
                {
                    value = it.name();
                    break;
                }
        }
        if (it != options.end())
            value = it->second;
    }
    else
    {
        /* This is a foo=true group. */

        for (auto& it : optionGroup.option())
            if (options.contains(it.name()))
            {
                value = it.name();
                break;
            }
        if (value.empty())
            for (auto& it : optionGroup.option())
                if (it.set_by_default())
                {
                    value = it.name();
                    break;
                }
    }

    ImGui::SeparatorText(name);
    std::string oldValue = value;
    for (auto& it : optionGroup.option())
    {
        if (ImGui::RadioButton(it.comment().c_str(), it.name() == value))
            value = it.name();
    }

    if (oldValue != value)
    {
        if (optionGroup.has_name())
            options[optionGroup.name()] = value;
        else
        {
            options.erase(oldValue);
            options[value] = "true";
        }
    }
}

static const std::string& renderFormatName(const std::string& id)
{
    auto it = formats.find(id);
    if (it == formats.end())
    {
        const static std::string UNKNOWN("Unknown");
        return UNKNOWN;
    }
    return it->second->shortname();
}

void ConfigView::drawContent()
{
    if (_configState == CONFIG_UNKNOWN)
        probeConfig();
    if (_configState != CONFIG_KNOWN)
        return;

    auto device_open =
        DynamicSetting<bool>("fluxengine.config.device", "open", true);
    ImGui::SetNextItemOpen(device_open);
    device_open = ImGui::CollapsingHeader(
        "fluxengine.view.config.deviceConfiguration"_lang);
    if (device_open)
    {
        int manualDriveIndex = _usbDevices.size() + 0;
        int fluxfileDriveIndex = _usbDevices.size() + 1;
        int selectedDrive = std::min(hex::ContentRegistry::Settings::read<int>(
                                         FLUXENGINE_CONFIG, SELECTED_DRIVE, 0),
            fluxfileDriveIndex);

        if (ImGui::BeginCombo("fluxengine.view.config.selectedDevice"_lang,
                "Preview",
                ImGuiComboFlags_HeightLargest))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (int i = 0; i < _usbDevices.size(); i++)
                if (ImGui::Selectable(
                        _usbDevices[i]->serial.c_str(), selectedDrive == i))
                {
                    selectedDrive = i;
                    hex::ContentRegistry::Settings::write<int>(
                        FLUXENGINE_CONFIG, SELECTED_DRIVE, selectedDrive);
                }

            if (ImGui::Selectable("fluxengine.view.config.manualDevice"_lang,
                    selectedDrive == manualDriveIndex))
            {
                selectedDrive = manualDriveIndex;
                hex::ContentRegistry::Settings::write<int>(
                    FLUXENGINE_CONFIG, SELECTED_DRIVE, selectedDrive);
            }
            if (ImGui::Selectable("fluxengine.view.config.fluxfileDevice"_lang,
                    selectedDrive == fluxfileDriveIndex))
            {
                selectedDrive = fluxfileDriveIndex;
                hex::ContentRegistry::Settings::write<int>(
                    FLUXENGINE_CONFIG, SELECTED_DRIVE, selectedDrive);
            }
        }

        std::set<int> applicableOptions{SOURCESINK};
        if (selectedDrive == manualDriveIndex)
        {
            ImGui::Text("manual drive config");
            applicableOptions.insert(MANUAL_SOURCESINK);
        }

        if (selectedDrive <= manualDriveIndex)
        {
            applicableOptions.insert(HARDWARE_SOURCESINK);
            bool highDensity = hex::ContentRegistry::Settings::read<bool>(
                FLUXENGINE_CONFIG, HIGH_DENSITY, 0);
            if (ImGui::Checkbox(
                    "fluxengine.view.config.highDensity"_lang, &highDensity))
                hex::ContentRegistry::Settings::write<bool>(
                    FLUXENGINE_CONFIG, HIGH_DENSITY, highDensity);
        }
        else if (selectedDrive == manualDriveIndex) {}
        else if (selectedDrive == fluxfileDriveIndex)
        {
            ImGui::Text("fluxfile drive config");
        }

        {
            auto globalOptions =
                DynamicSetting<std::string>("fluxengine.config", "global", "");
            OptionsMap parsedOptions = stringToOptions(globalOptions);

            const auto& config = formats.at("_global_options");
            if (!config->option().empty())
            {
                ImGui::SeparatorText("fluxengine.view.config.other"_lang);
                for (auto& it : config->option())
                {
                    if (std::ranges::any_of(it.applicability(),
                            [&](int item)
                            {
                                return applicableOptions.contains(item);
                            }))
                        showOption(parsedOptions, it);
                }
            }

            for (auto& it : config->option_group())
            {
                if (std::ranges::any_of(it.applicability(),
                        [&](int item)
                        {
                            return applicableOptions.contains(item);
                        }))
                    showOptionGroup(parsedOptions, it);
            }

            globalOptions = optionsToString(parsedOptions);
        }
    }

    auto format_open =
        DynamicSetting<bool>("fluxengine.config.format", "open", true);
    ImGui::Spacing();
    ImGui::SetNextItemOpen(format_open);
    format_open = ImGui::CollapsingHeader(
        "fluxengine.view.config.formatConfiguration"_lang);
    if (format_open)
    {
        auto formatSetting =
            DynamicSetting<std::string>("fluxengine.settings.format", "selected", "ibm");
        if (ImGui::BeginCombo("fluxengine.view.config.selectedFormat"_lang,
                renderFormatName(formatSetting).c_str(),
                ImGuiComboFlags_HeightLargest))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& [id, proto] : formats)
            {
                if (!proto->is_extension())
                {
                    auto shortname = proto->shortname();
                    if (shortname.empty())
                        shortname = id;

                    if (ImGui::Selectable(shortname.c_str(),
                            (std::string)formatSetting == id))
                        formatSetting = id;
                }
            }
        }

        {
            auto globalOptions = DynamicSetting<std::string>(
                "fluxengine.settings", (std::string)formatSetting, "");
            OptionsMap parsedOptions = stringToOptions(globalOptions);

            auto config = formats.at(formatSetting);
            if (!config->option().empty())
            {
                ImGui::SeparatorText("fluxengine.view.config.other"_lang);
                for (auto& it : config->option())
                    showOption(parsedOptions, it);
            }

            for (auto& it : config->option_group())
            {
                if (it.applicability().empty() ||
                    std::ranges::contains(it.applicability(), FORMAT))
                {
                    auto name = it.comment();
                    if (name == "$formats")
                        name = (std::
                                string) "fluxengine.view.config.subformats"_lang;
                    showOptionGroup(parsedOptions, it, name.c_str());
                }
            }

            globalOptions = optionsToString(parsedOptions);
        }
    }
}

void ConfigView::probeConfig()
{
    _configState = CONFIG_PROBING;
    Datastore::runOnWorkerThread(
        [this]
        {
            hex::log::debug("probing USB");
            _usbDevices = findUsbDevices();

            _configState = CONFIG_KNOWN;
        });
}
