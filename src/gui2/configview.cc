#include <hex/api/content_registry/user_interface.hpp>
#include <hex/api/theme_manager.hpp>
#include <hex/helpers/logger.hpp>
#include <fonts/vscode_icons.hpp>
#include <fonts/tabler_icons.hpp>
#include <fmt/format.h>
#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/decoded.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "globals.h"
#include "configview.h"
#include "datastore.h"
#include "utils.h"

using namespace hex;

static DynamicSettingFactory settings("fluxengine.settings");

ConfigView::ConfigView():
    View::Window("fluxengine.view.config.name", ICON_VS_COMPASS)
{
}

static void emitOptions(DynamicSetting<std::string>& setting,
    const ConfigProto* config,
    const std::set<int>& applicableOptions)
{
    auto options = stringToOptions(setting);

    for (auto& it : config->option())
    {
        if (!applicableOptions.empty() &&
            std::ranges::none_of(it.applicability(),
                [&](int item)
                {
                    return applicableOptions.contains(item);
                }))
            continue;

        bool selected = options.contains(it.name());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextWrapped(
            "%s", wolv::util::capitalizeString(it.comment()).c_str());

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo(fmt::format("##{}", it.comment()).c_str(),
                selected ? "fluxengine.view.config.yes"_lang
                         : "fluxengine.view.config.no"_lang,
                ImGuiComboFlags_None))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            int value = -1;
            if (ImGui::Selectable("fluxengine.view.config.no"_lang, !selected))
                value = 0;
            if (ImGui::Selectable("fluxengine.view.config.yes"_lang, selected))
                value = 1;
            if (value != -1)
            {
                options.erase(it.name());
                if (value)
                    options[it.name()] = "true";
                setting = optionsToString(options);
                Datastore::reset();
            }
        }
    }

    for (auto& it : config->option_group())
    {
        if (!applicableOptions.empty() &&
            std::ranges::none_of(it.applicability(),
                [&](int item)
                {
                    return applicableOptions.contains(item);
                }))
            continue;

        std::string comment = it.comment();
        if (comment == "$formats")
            comment = (std::string) "fluxengine.view.config.variations"_lang;

        const OptionProto* selectedOption = nullptr;
        if (it.name().empty())
        {
            for (auto& ot : it.option())
                if (options.contains(ot.name()))
                    selectedOption = &ot;
        }
        else
        {
            auto value = findOrDefault(options, it.name());
            for (auto& ot : it.option())
                if (ot.name() == value)
                    selectedOption = &ot;
        }

        if (!selectedOption)
            for (auto& ot : it.option())
                if (ot.set_by_default())
                    selectedOption = &ot;

        std::string value;
        if (selectedOption)
            value = wolv::util::capitalizeString(selectedOption->comment());
        else
            value = "***missing default***";

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextWrapped("%s", comment.c_str());

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo(fmt::format("##{}", it.comment()).c_str(),
                value.c_str(),
                ImGuiComboFlags_None))
        {
            ON_SCOPE_EXIT
            {
                ImGui::EndCombo();
            };

            for (auto& ot : it.option())
                if (ImGui::Selectable(
                        ot.comment().c_str(), &ot == selectedOption))
                {
                    if (it.name().empty())
                    {
                        if (selectedOption)
                            options.erase(selectedOption->name());
                        options[ot.name()] = "true";
                    }
                    else
                    {
                        options.erase(it.name());
                        options[it.name()] = ot.name();
                    }
                    setting = optionsToString(options);
                    Datastore::reset();
                }
        }
    }
}

static void formatProperties()
{
    auto formatSetting = settings.get<std::string>("format.selected", "ibm");
    auto selectedFormat = findOrDefault(formats, (std::string)formatSetting);
    if (!selectedFormat)
    {
        formatSetting = "ibm";
        selectedFormat = formats.at(formatSetting);
    }
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextWrapped("Format");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##formats",
            selectedFormat->shortname().c_str(),
            ImGuiComboFlags_HeightLargest))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndCombo();
        };

        for (auto& [name, format] : formats)
            if (!format->is_extension())
            {
                auto label = format->shortname();
                if (label.empty())
                    label = name;
                if (ImGui::Selectable(
                        fmt::format("{}##{}", label, name).c_str(), false))
                {
                    formatSetting = name;
                    Datastore::reset();
                }
            }
    }

    auto formatOptionsSetting = DynamicSetting<std::string>(
        "fluxengine.settings", (std::string)formatSetting);
    emitOptions(formatOptionsSetting, selectedFormat, {});
}

static void deviceProperties()
{
    /* Device name */

    std::set<int> applicableOptions = {ANY_SOURCESINK};
    auto deviceNameSetting = settings.get<std::string>("device");
    auto selectedDevice = findOrDefault(Datastore::getDevices(),
        (std::string)deviceNameSetting,
        {.label = "No device configured"});
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Device");

    if ((std::string)deviceNameSetting == DEVICE_FLUXFILE)
        applicableOptions.insert(FLUXFILE_SOURCESINK);
    else if ((std::string)deviceNameSetting == DEVICE_MANUAL)
    {
        applicableOptions.insert(MANUAL_SOURCESINK);
        applicableOptions.insert(HARDWARE_SOURCESINK);
    }
    else
        applicableOptions.insert(HARDWARE_SOURCESINK);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##devices",
            selectedDevice.label.c_str(),
            ImGuiComboFlags_HeightLargest))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndCombo();
        };

        for (auto& [name, device] : Datastore::getDevices())
            if (ImGui::Selectable(device.label.c_str(), false))
                deviceNameSetting = name;
    }

    /* The rescan button. */

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
    if (ImGui::Button("fluxengine.view.config.rescan"_lang))
    {
        Datastore::probeDevices();
    }

    ImGui::SameLine();
    if (ImGui::Button("fluxengine.view.config.setupFluxFile"_lang))
    {
        fs::openFileBrowser(fs::DialogMode::Open,
            {},
            [](const auto& path)
            {
                settings.get<std::string>("device") = DEVICE_FLUXFILE;
                settings.get<std::fs::path>("fluxfile") = path;
            });
    }

    /* The file path, if DEVICE_FLUXFILE, and device path, if DEVICE_MANUAL
     */

    auto doPathSetting = [=](const std::string& label,
                             const std::string& setting,
                             const std::string& id)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(label);

        auto pathSetting = settings.get<std::string>(setting);
        auto pathString = (std::string)pathSetting;
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText(id.c_str(),
                pathString,
                ImGuiInputTextFlags_CallbackCompletion |
                    ImGuiInputTextFlags_ElideLeft))
            pathSetting = pathString;
    };

    if ((std::string)deviceNameSetting == DEVICE_FLUXFILE)
        doPathSetting("fluxengine.view.config.fluxFile"_lang,
            "fluxfile",
            "##fluxfilePath");
    if ((std::string)deviceNameSetting == DEVICE_MANUAL)
        doPathSetting("fluxengine.view.config.manualDevicePath"_lang,
            "manualDevicePath",
            "##manualDevicePath");

    /* Other options */

    auto globalOptionsSetting =
        DynamicSetting<std::string>("fluxengine.settings", "globalSettings");
    emitOptions(
        globalOptionsSetting, formats.at("_global_options"), applicableOptions);
}

void ConfigView::drawContent()
{
    ImGui::BeginDisabled(Datastore::isBusy());
    ON_SCOPE_EXIT
    {
        ImGui::EndDisabled();
    };

    ImGui::SeparatorText("fluxengine.view.config.formatProperties"_lang);
    if (ImGui::BeginTable("propertiesEditor",
            2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndTable();
        };

        formatProperties();
    }

    ImGui::SeparatorText("fluxengine.view.config.deviceProperties"_lang);
    if (ImGui::BeginTable("propertiesEditor",
            2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg))
    {
        ON_SCOPE_EXIT
        {
            ImGui::EndTable();
        };
        deviceProperties();
    }

    ImGui::SeparatorText("fluxengine.view.config.customProperties"_lang);

    auto customSetting = settings.get<std::string>("custom");
    std::string buffer = customSetting;
    if (ImGui::InputTextMultiline("##customProperties",
            buffer,
            ImGui::GetContentRegionAvail(),
            ImGuiInputTextFlags_None))
        customSetting = buffer;
}
