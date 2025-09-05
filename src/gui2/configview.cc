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
#include "utils.h"
#include "configview.h"
#include "datastore.h"

/* Imported from src/gui/drivetypes */
extern const std::map<std::string, const ConfigProto*> drivetypes;

using namespace hex;

const hex::UnlocalizedString FLUXENGINE_CONFIG("fluxengine.config");

static const hex::UnlocalizedString SELECTED_DRIVE(
    "fluxengine.config.selected_drive");
static const hex::UnlocalizedString HIGH_DENSITY(
    "fluxengine.config.high_density");

static const std::string DEVICE_MANUAL = "manual";
static const std::string DEVICE_FLUXFILE = "fluxfile";

namespace
{
}

ConfigView::ConfigView():
    View::Window("fluxengine.view.config.name", ICON_VS_DEBUG_LINE_BY_LINE)
{
}

static void showOption(OptionsMap& options, const OptionProto& option)
{
    const auto& name = option.name();
    auto it = options.find(name);
    bool value = false;
    if (it != options.end())
        value = true;

    bool oldValue = value;
    ImGui::Checkbox(option.comment().c_str(), &value);
    if (oldValue != value)
    {
        if (value)
            options[name] = "";
        else
            options.erase(name);
    }
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

    ImGui::BeginDisabled(Datastore::isBusy());
    ON_SCOPE_EXIT
    {
        ImGui::EndDisabled();
    };

    auto device_open =
        DynamicSetting<bool>("fluxengine.config.device", "open", true);
    ImGui::SetNextItemOpen(device_open);
    device_open = ImGui::CollapsingHeader(
        "fluxengine.view.config.deviceConfiguration"_lang);
    if (device_open)
    {
        auto driveSetting = DynamicSetting<std::string>(
            "fluxengine.settings", "device", DEVICE_FLUXFILE);
        if (!_devices->contains(driveSetting))
            driveSetting = DEVICE_FLUXFILE;

        if (ImGui::BeginCombo("fluxengine.view.config.selectedDevice"_lang,
                _devices->at(driveSetting).label.c_str(),
                ImGuiComboFlags_HeightLargest))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& it : *_devices)
            {
                if (ImGui::Selectable(it.second.label.c_str(),
                        (std::string)driveSetting == it.first))
                    driveSetting = it.first;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("fluxengine.view.config.rescan"_lang))
        {
            _configState = CONFIG_UNKNOWN;
            return;
        }

        std::set<int> applicableOptions{SOURCESINK};
        if ((std::string)driveSetting == DEVICE_MANUAL)
        {
            auto manualDevicePathSetting = DynamicSetting<std::fs::path>(
                "fluxengine.settings", "manualDevicePath", "");
            std::fs::path value = manualDevicePathSetting;
            if (ImGuiExt::InputFilePicker(
                    "fluxengine.view.config.hardwareDevicePath"_lang,
                    value,
                    {}))
                manualDevicePathSetting = value;

            applicableOptions.insert(MANUAL_SOURCESINK);
        }

        if (((std::string)driveSetting == DEVICE_MANUAL) ||
            (((std::string)driveSetting)[0] == '#'))
        {
            auto highdensitySetting = DynamicSetting<bool>(
                "fluxengine.settings", "highDensity", true);

            bool value = highdensitySetting;
            if (ImGui::Checkbox(
                    "fluxengine.view.config.highDensity"_lang, &value))
                highdensitySetting = value;

            applicableOptions.insert(HARDWARE_SOURCESINK);
        }
        else if ((std::string)driveSetting == DEVICE_FLUXFILE)
        {
            auto fluxfilePathSetting = DynamicSetting<std::fs::path>(
                "fluxengine.settings", "fluxfile", "");
            std::fs::path value = fluxfilePathSetting;
            if (ImGuiExt::InputFilePicker(
                    "fluxengine.view.config.fluxfile"_lang, value, {}))
                fluxfilePathSetting = value;
        }

        {
            auto globalOptions = DynamicSetting<std::string>(
                "fluxengine.settings", "globalSettings", "");
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
        auto formatSetting = DynamicSetting<std::string>(
            "fluxengine.settings.format", "selected", "ibm");
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
            auto usbDevices = findUsbDevices();
            auto devices = std::make_shared<DeviceMap>();

            (*devices)[DEVICE_FLUXFILE] = {
                nullptr, "fluxengine.view.config.fluxfile"_lang};
            (*devices)[DEVICE_MANUAL] = {
                nullptr, "fluxengine.view.config.manual"_lang};
            for (auto it : usbDevices)
                (*devices)["#" + it->serial] = {it,
                    fmt::format("{}: {}", getDeviceName(it->type), it->serial)};

            hex::TaskManager::doLater(
                [this, devices]
                {
                    _devices = devices;
                    _configState = CONFIG_KNOWN;
                });
        });
}
