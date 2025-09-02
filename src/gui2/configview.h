#pragma once

#include <hex/ui/view.hpp>

class CandidateDevice;

class ConfigView : public hex::View::Window {
private:
    enum ConfigState {
        CONFIG_UNKNOWN,
        CONFIG_PROBING,
        CONFIG_KNOWN
    };

public:
    ConfigView();
    ~ConfigView() override = default;

    void drawContent() override;

    [[nodiscard]] bool shouldDraw() const override { return true; }
    [[nodiscard]] bool hasViewMenuItemEntry() const override { return true; }

private:
    void probeConfig();

private:
    int _driveIndex;
    
    ConfigState _configState = CONFIG_UNKNOWN;
    std::vector<std::shared_ptr<CandidateDevice>> _usbDevices;
};

