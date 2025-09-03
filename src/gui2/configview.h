#pragma once

#include <hex/ui/view.hpp>

class CandidateDevice;

class ConfigView : public hex::View::Window
{
private:
    enum ConfigState
    {
        CONFIG_UNKNOWN,
        CONFIG_PROBING,
        CONFIG_KNOWN
    };

public:
    ConfigView();
    ~ConfigView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override
    {
        return true;
    }
    [[nodiscard]] bool hasViewMenuItemEntry() const override
    {
        return true;
    }

private:
    void probeConfig();

private:
    int _driveIndex;

    struct Device
    {
        std::shared_ptr<CandidateDevice> coreDevice;
        std::string label;
    };

    typedef std::map<std::string, Device> DeviceMap;

    ConfigState _configState = CONFIG_UNKNOWN;
    std::shared_ptr<DeviceMap> _devices;
    std::fs::path _manualDevicePath;
    std::fs::path _fluxfilePath;
};
