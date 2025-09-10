#pragma once

#include <hex/ui/view.hpp>

class CandidateDevice;

class ConfigView : public hex::View::Window
{
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

    std::shared_ptr<DeviceMap> _devices;
};

static constexpr std::string DEVICE_MANUAL = "manual";
static constexpr std::string DEVICE_FLUXFILE = "fluxfile";
